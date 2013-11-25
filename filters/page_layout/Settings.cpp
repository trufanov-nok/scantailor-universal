/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Settings.h"
#include "PageId.h"
#include "PageSequence.h"
#include "Params.h"
#include "RelativeMargins.h"
#include "Alignment.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include <QSizeF>
#include <QMutex>
#include <QMutexLocker>
#include <boost/foreach.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <algorithm>
#include <functional> // for std::greater<>
#include <vector>
#include <stddef.h>


using namespace ::boost;
using namespace ::boost::multi_index;

namespace page_layout
{

class Settings::Item
{
public:
	PageId pageId;
	RelativeMargins hardMargins;
	QSizeF contentSize;
	MatchSizeMode matchSizeMode;
	Alignment alignment;
	
	Item(PageId const& page_id, RelativeMargins const& hard_margins,
		QSizeF const& content_size, MatchSizeMode const& match_size_mode,
		Alignment const& alignment);
	
	double hardWidth() const;
	
	double hardHeight() const;
	
	/** This page's "widest page" competition entry. */
	double influenceHardWidth() const;
	
	/** This page's "tallest page" competition entry. */
	double influenceHardHeight() const;
	
	/**
	 * When sorted by width or height, we put pages with
	 * "alignedWithOthers() == false" to the bottom of the list.
	 */
	bool alignedWithOthers() const {
		return matchSizeMode.get() != MatchSizeMode::DISABLED;
	}
};


class Settings::ModifyMargins
{
public:
	ModifyMargins(RelativeMargins const& margins) : m_margins(margins) {}
	
	void operator()(Item& item) {
		item.hardMargins = m_margins;
	}
private:
	RelativeMargins m_margins;
};


class Settings::ModifyAlignment
{
public:
	ModifyAlignment(Alignment const& alignment) : m_alignment(alignment) {}
	
	void operator()(Item& item) {
		item.alignment = m_alignment;
	}
private:
	Alignment m_alignment;
};


class Settings::ModifyContentSize
{
public:
	ModifyContentSize(QSizeF const& content_size)
	: m_contentSize(content_size) {}
	
	void operator()(Item& item) {
		item.contentSize = m_contentSize;
	}
private:
	QSizeF m_contentSize;
};


class Settings::Impl
{
public:
	Impl();
	
	~Impl();
	
	void clear();

	void performRelinking(AbstractRelinker const& relinker);
	
	void removePagesMissingFrom(PageSequence const& pages);
	
	bool checkEverythingDefined(
		PageSequence const& pages, PageId const* ignore) const;
	
	std::auto_ptr<Params> getPageParams(PageId const& page_id) const;
	
	void setPageParams(PageId const& page_id, Params const& params);
	
	Params updateContentSizeAndGetParams(
		PageId const& page_id, QSizeF const& content_size,
		QSizeF* agg_hard_size_before, QSizeF* agg_hard_size_after);
	
	RelativeMargins getHardMargins(PageId const& page_id) const;
	
	void setHardMargins(PageId const& page_id, RelativeMargins const& margins);

	MatchSizeMode getMatchSizeMode(PageId const& page_id) const;

	AggregateSizeChanged setMatchSizeMode(
		PageId const& page_id, MatchSizeMode const& match_size_mode);
	
	Alignment getPageAlignment(PageId const& page_id) const;
	
	void setPageAlignment(
		PageId const& page_id, Alignment const& alignment);
	
	AggregateSizeChanged setContentSize(
		PageId const& page_id, QSizeF const& content_size);
	
	void invalidateContentSize(PageId const& page_id);
	
	QSizeF getAggregateHardSize() const;
	
	QSizeF getAggregateHardSizeLocked() const;
	
	QSizeF getAggregateHardSize(
		PageId const& page_id, QSizeF const& hard_size,
		MatchSizeMode const& match_size_mode) const;
private:
	class SequencedTag;
	class DescWidthTag;
	class DescHeightTag;
	
	typedef multi_index_container<
		Item,
		indexed_by<
			ordered_unique<member<Item, PageId, &Item::pageId> >,
			sequenced<tag<SequencedTag> >,
			ordered_non_unique<
				tag<DescWidthTag>,
				// ORDER BY alignedWithOthers DESC, hardWidth DESC
				composite_key<
					Item,
					const_mem_fun<Item, bool, &Item::alignedWithOthers>,
					const_mem_fun<Item, double, &Item::hardWidth>
				>,
				composite_key_compare<
					std::greater<bool>,
					std::greater<double>
				>
			>,
			ordered_non_unique<
				tag<DescHeightTag>,
				// ORDER BY alignedWithOthers DESC, hardHeight DESC
				composite_key<
					Item,
					const_mem_fun<Item, bool, &Item::alignedWithOthers>,
					const_mem_fun<Item, double, &Item::hardHeight>
				>,
				composite_key_compare<
					std::greater<bool>,
					std::greater<double>
				>
			>
		>
	> Container;
	
	typedef Container::index<SequencedTag>::type UnorderedItems;
	typedef Container::index<DescWidthTag>::type DescWidthOrder;
	typedef Container::index<DescHeightTag>::type DescHeightOrder;
	
	mutable QMutex m_mutex;
	Container m_items;
	UnorderedItems& m_unorderedItems;
	DescWidthOrder& m_descWidthOrder;
	DescHeightOrder& m_descHeightOrder;
	QSizeF const m_invalidSize;
	RelativeMargins const m_defaultHardMargins;
	MatchSizeMode const m_defaultMatchSizeMode;
	Alignment const m_defaultAlignment;
};


/*=============================== Settings ==================================*/

Settings::Settings()
:	m_ptrImpl(new Impl())
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	return m_ptrImpl->clear();
}

void
Settings::performRelinking(AbstractRelinker const& relinker)
{
	m_ptrImpl->performRelinking(relinker);
}

void
Settings::removePagesMissingFrom(PageSequence const& pages)
{
	m_ptrImpl->removePagesMissingFrom(pages);
}

bool
Settings::checkEverythingDefined(
	PageSequence const& pages, PageId const* ignore) const
{
	return m_ptrImpl->checkEverythingDefined(pages, ignore);
}

std::auto_ptr<Params>
Settings::getPageParams(PageId const& page_id) const
{
	return m_ptrImpl->getPageParams(page_id);
}

void
Settings::setPageParams(PageId const& page_id, Params const& params)
{
	return m_ptrImpl->setPageParams(page_id, params);
}

Params
Settings::updateContentSizeAndGetParams(
	PageId const& page_id, QSizeF const& content_size,
	QSizeF* agg_hard_size_before, QSizeF* agg_hard_size_after)
{
	return m_ptrImpl->updateContentSizeAndGetParams(
		page_id, content_size,
		agg_hard_size_before, agg_hard_size_after
	);
}

RelativeMargins
Settings::getHardMargins(PageId const& page_id) const
{
	return m_ptrImpl->getHardMargins(page_id);
}

void
Settings::setHardMargins(PageId const& page_id, RelativeMargins const& margins)
{
	m_ptrImpl->setHardMargins(page_id, margins);
}

MatchSizeMode
Settings::getMatchSizeMode(PageId const& page_id) const
{
	return m_ptrImpl->getMatchSizeMode(page_id);
}

Settings::AggregateSizeChanged
Settings::setMatchSizeMode(PageId const& page_id, MatchSizeMode const& match_size_mode)
{
	return m_ptrImpl->setMatchSizeMode(page_id, match_size_mode);
}

Alignment
Settings::getPageAlignment(PageId const& page_id) const
{
	return m_ptrImpl->getPageAlignment(page_id);
}

void
Settings::setPageAlignment(PageId const& page_id, Alignment const& alignment)
{
	m_ptrImpl->setPageAlignment(page_id, alignment);
}

Settings::AggregateSizeChanged
Settings::setContentSize(
	PageId const& page_id, QSizeF const& content_size)
{
	return m_ptrImpl->setContentSize(page_id, content_size);
}

void
Settings::invalidateContentSize(PageId const& page_id)
{
	return m_ptrImpl->invalidateContentSize(page_id);
}

QSizeF
Settings::getAggregateHardSize() const
{
	return m_ptrImpl->getAggregateHardSize();
}

QSizeF
Settings::getAggregateHardSize(
	PageId const& page_id, QSizeF const& hard_size,
	MatchSizeMode const& match_size_mode) const
{
	return m_ptrImpl->getAggregateHardSize(page_id, hard_size, match_size_mode);
}


/*============================== Settings::Item =============================*/

Settings::Item::Item(
	PageId const& page_id, RelativeMargins const& hard_margins,
	QSizeF const& content_size, MatchSizeMode const& match_size_mode,
	Alignment const& align)
:	pageId(page_id),
	hardMargins(hard_margins),
	contentSize(content_size),
	matchSizeMode(match_size_mode),
	alignment(align)
{
}

double
Settings::Item::hardWidth() const
{
	return hardMargins.extendContentSize(contentSize).width();
}

double
Settings::Item::hardHeight() const
{
	return hardMargins.extendContentSize(contentSize).height();
}

double
Settings::Item::influenceHardWidth() const
{
	return matchSizeMode.get() == MatchSizeMode::DISABLED ? 0.0 : hardWidth();
}

double
Settings::Item::influenceHardHeight() const
{
	return matchSizeMode.get() == MatchSizeMode::DISABLED ? 0.0 : hardHeight();
}


/*============================= Settings::Impl ==============================*/

Settings::Impl::Impl()
:	m_items(),
	m_unorderedItems(m_items.get<SequencedTag>()),
	m_descWidthOrder(m_items.get<DescWidthTag>()),
	m_descHeightOrder(m_items.get<DescHeightTag>()),
	m_invalidSize(),
	m_defaultHardMargins(page_layout::Settings::defaultHardMargins()),
	m_defaultMatchSizeMode(MatchSizeMode::GROW_MARGINS),
	m_defaultAlignment(Alignment::TOP, Alignment::HCENTER)
{
}

Settings::Impl::~Impl()
{
}

void
Settings::Impl::clear()
{
	QMutexLocker const locker(&m_mutex);
	m_items.clear();
}

void
Settings::Impl::performRelinking(AbstractRelinker const& relinker)
{
	QMutexLocker locker(&m_mutex);
	Container new_items;

	BOOST_FOREACH(Item const& item, m_unorderedItems) {
		RelinkablePath const old_path(item.pageId.imageId().filePath(), RelinkablePath::File);
		Item new_item(item);
		new_item.pageId.imageId().setFilePath(relinker.substitutionPathFor(old_path));
		new_items.insert(new_item);
	}

	m_items.swap(new_items);
}

void
Settings::Impl::removePagesMissingFrom(PageSequence const& pages)
{
	QMutexLocker const locker(&m_mutex);

	std::vector<PageId> sorted_pages;
	size_t const num_pages = pages.numPages();
	sorted_pages.reserve(num_pages);
	for (size_t i = 0; i < num_pages; ++i) {
		sorted_pages.push_back(pages.pageAt(i).id());
	}
	std::sort(sorted_pages.begin(), sorted_pages.end());

	UnorderedItems::const_iterator it(m_unorderedItems.begin());
	UnorderedItems::const_iterator const end(m_unorderedItems.end());
	while (it != end) {
		if (std::binary_search(sorted_pages.begin(), sorted_pages.end(), it->pageId)) {
			++it;
		} else {
			m_unorderedItems.erase(it++);
		}
	}
}

bool
Settings::Impl::checkEverythingDefined(
	PageSequence const& pages, PageId const* ignore) const
{
	QMutexLocker const locker(&m_mutex);
	
	size_t const num_pages = pages.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info = pages.pageAt(i);
		if (ignore && *ignore == page_info.id()) {
			continue;
		}
		Container::iterator const it(m_items.find(page_info.id()));
		if (it == m_items.end() || !it->contentSize.isValid()) {
			return false;
		}
	}
	
	return true;
}

std::auto_ptr<Params>
Settings::Impl::getPageParams(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.find(page_id));
	if (it == m_items.end()) {
		return std::auto_ptr<Params>();
	}
	
	return std::auto_ptr<Params>(
		new Params(it->hardMargins, it->contentSize, it->matchSizeMode, it->alignment)
	);
}

void
Settings::Impl::setPageParams(PageId const& page_id, Params const& params)
{
	QMutexLocker const locker(&m_mutex);
	
	Item const new_item(
		page_id, params.hardMargins(), params.contentSize(),
		params.matchSizeMode(), params.alignment()
	);
	
	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		m_items.insert(it, new_item);
	} else {
		m_items.replace(it, new_item);
	}
}

Params
Settings::Impl::updateContentSizeAndGetParams(
	PageId const& page_id, QSizeF const& content_size,
	QSizeF* agg_hard_size_before, QSizeF* agg_hard_size_after)
{
	QMutexLocker const locker(&m_mutex);
	
	if (agg_hard_size_before) {
		*agg_hard_size_before = getAggregateHardSizeLocked();
	}
	
	Container::iterator const it(m_items.lower_bound(page_id));
	Container::iterator item_it(it);
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, m_defaultHardMargins, content_size,
			m_defaultMatchSizeMode, m_defaultAlignment
		);
		item_it = m_items.insert(it, item);
	} else {
		m_items.modify(it, ModifyContentSize(content_size));
	}
	
	if (agg_hard_size_after) {
		*agg_hard_size_after = getAggregateHardSizeLocked();
	}
	
	return Params(
		item_it->hardMargins, item_it->contentSize,
		item_it->matchSizeMode, item_it->alignment
	);
}

RelativeMargins
Settings::Impl::getHardMargins(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.find(page_id));
	if (it == m_items.end()) {
		return m_defaultHardMargins;
	} else {
		return it->hardMargins;
	}
}

void
Settings::Impl::setHardMargins(
	PageId const& page_id, RelativeMargins const& margins)
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, margins, m_invalidSize,
			m_defaultMatchSizeMode, m_defaultAlignment
		);
		m_items.insert(it, item);
	} else {
		m_items.modify(it, ModifyMargins(margins));
	}
}

MatchSizeMode
Settings::Impl::getMatchSizeMode(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);

	Container::iterator const it(m_items.find(page_id));
	if (it == m_items.end()) {
		return m_defaultMatchSizeMode;
	} else {
		return it->matchSizeMode;
	}
}

Settings::AggregateSizeChanged
Settings::Impl::setMatchSizeMode(
	PageId const& page_id, MatchSizeMode const& match_size_mode)
{
	QMutexLocker const locker(&m_mutex);

	QSizeF const agg_size_before(getAggregateHardSizeLocked());

	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, m_defaultHardMargins, m_invalidSize,
			match_size_mode, m_defaultAlignment
		);
		m_items.insert(it, item);
	} else {
		m_items.modify(it, [match_size_mode](Item& item) {
			item.matchSizeMode = match_size_mode;
		});
	}

	QSizeF const agg_size_after(getAggregateHardSizeLocked());
	if (agg_size_before == agg_size_after) {
		return AGGREGATE_SIZE_UNCHANGED;
	} else {
		return AGGREGATE_SIZE_CHANGED;
	}
}

Alignment
Settings::Impl::getPageAlignment(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.find(page_id));
	if (it == m_items.end()) {
		return m_defaultAlignment;
	} else {
		return it->alignment;
	}
}

void
Settings::Impl::setPageAlignment(
	PageId const& page_id, Alignment const& alignment)
{
	QMutexLocker const locker(&m_mutex);

	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, m_defaultHardMargins, m_invalidSize,
			m_defaultMatchSizeMode, alignment
		);
		m_items.insert(it, item);
	} else {
		m_items.modify(it, ModifyAlignment(alignment));
	}
}

Settings::AggregateSizeChanged
Settings::Impl::setContentSize(
	PageId const& page_id, QSizeF const& content_size)
{
	QMutexLocker const locker(&m_mutex);
	
	QSizeF const agg_size_before(getAggregateHardSizeLocked());
	
	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, m_defaultHardMargins, content_size,
			m_defaultMatchSizeMode, m_defaultAlignment
		);
		m_items.insert(it, item);
	} else {
		m_items.modify(it, ModifyContentSize(content_size));
	}
	
	QSizeF const agg_size_after(getAggregateHardSizeLocked());
	if (agg_size_before == agg_size_after) {
		return AGGREGATE_SIZE_UNCHANGED;
	} else {
		return AGGREGATE_SIZE_CHANGED;
	}
}

void
Settings::Impl::invalidateContentSize(PageId const& page_id)
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.find(page_id));
	if (it != m_items.end()) {
		m_items.modify(it, ModifyContentSize(m_invalidSize));
	}
}

QSizeF
Settings::Impl::getAggregateHardSize() const
{
	QMutexLocker const locker(&m_mutex);
	return getAggregateHardSizeLocked();
}

QSizeF
Settings::Impl::getAggregateHardSizeLocked() const
{
	if (m_items.empty()) {
		return QSizeF(0.0, 0.0);
	}
	
	Item const& max_width_item = *m_descWidthOrder.begin();
	Item const& max_height_item = *m_descHeightOrder.begin();
	
	double const width = max_width_item.influenceHardWidth();
	double const height = max_height_item.influenceHardHeight();
	
	return QSizeF(width, height);
}

QSizeF
Settings::Impl::getAggregateHardSize(
	PageId const& page_id, QSizeF const& hard_size,
	MatchSizeMode const& match_size_mode) const
{
	QSizeF influence_hard_size = QSize(0.0, 0.0);
	if (match_size_mode.get() != MatchSizeMode::DISABLED) {
		influence_hard_size = hard_size;
	}
	
	QMutexLocker const locker(&m_mutex);
	
	if (m_items.empty()) {
		return QSizeF(0.0, 0.0);
	}
	
	qreal width = 0.0;
	
	{
		DescWidthOrder::iterator it(m_descWidthOrder.begin());
		if (it->pageId == page_id) {
			++it;
		}

		if (it == m_descWidthOrder.end()) {
			width = influence_hard_size.width();
		} else {
			width = std::max(influence_hard_size.width(), qreal(it->influenceHardWidth()));
		}
	}
	
	qreal height = 0.0;
	
	{
		DescHeightOrder::iterator it(m_descHeightOrder.begin());
		if (it->pageId == page_id) {
			++it;
		}

		if (it == m_descHeightOrder.end()) {
			height = influence_hard_size.height();
		} else {
			height = std::max(influence_hard_size.height(), qreal(it->influenceHardHeight()));
		}
	}
	
	return QSizeF(width, height);
}

} // namespace page_layout
