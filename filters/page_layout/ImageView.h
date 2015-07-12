/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef PAGE_LAYOUT_IMAGEVIEW_H_
#define PAGE_LAYOUT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "ImagePixmapUnion.h"
#include "InteractionHandler.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "DraggableObject.h"
#include "ObjectDragHandler.h"
#include "MatchSizeMode.h"
#include "Alignment.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "imageproc/AffineImageTransform.h"
#include "acceleration/AcceleratableOperations.h"
#include <QTransform>
#include <QSizeF>
#include <QRectF>
#include <QPointF>
#include <QPoint>
#include <memory>

class ContentBox;
class RelativeMargins;

namespace imageproc
{
	class AffineTransformedImage;
}

namespace page_layout
{

class OptionsWidget;
class Settings;

class ImageView :
	public ImageViewBase,
	private InteractionHandler
{
	Q_OBJECT
public:
	ImageView(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		IntrusivePtr<Settings> const& settings, PageId const& page_id,
		std::shared_ptr<imageproc::AbstractImageTransform const> const& orig_transform,
		imageproc::AffineTransformedImage const& affine_transformed_image,
		ImagePixmapUnion const& downscaled_image,
		ContentBox const& content_box,
		OptionsWidget const& opt_widget);
	
	virtual ~ImageView();
signals:
	void invalidateThumbnail(PageId const& page_id);
	
	void invalidateAllThumbnails();
	
	void marginsSetLocally(RelativeMargins const& margins);
public slots:
	void marginsSetExternally(RelativeMargins const& margins);
	
	void leftRightLinkToggled(bool linked);
	
	void topBottomLinkToggled(bool linked);
	
	void matchSizeModeChanged(MatchSizeMode const& match_size_mode);

	void alignmentChanged(Alignment const& alignment);
	
	void aggregateHardSizeChanged();
private:
	enum Edge { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };
	enum FitMode { FIT, DONT_FIT };
	enum AggregateSizeChanged { AGGREGATE_SIZE_UNCHANGED, AGGREGATE_SIZE_CHANGED };
	
	struct StateBeforeResizing
	{
		/**
		 * Transformation from virtual image coordinates to widget coordinates.
		 */
		QTransform virtToWidget;
		
		/**
		 * Transformation from widget coordinates to virtual image coordinates.
		 */
		QTransform widgetToVirt;
		
		/**
		 * m_middleRect in widget coordinates.
		 */
		QRectF middleWidgetRect;
		
		/**
		 * Mouse pointer position in widget coordinates.
		 */
		QPointF mousePos;
		
		/**
		 * The point in image that is to be centered on the screen,
		 * in pixel image coordinates.
		 */
		QPointF focalPoint;
	};
	
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	Proximity cornerProximity(int edge_mask, QRectF const* box, QPointF const& mouse_pos) const;

	Proximity edgeProximity(int edge_mask, QRectF const* box, QPointF const& mouse_pos) const;

	void dragInitiated(QPointF const& mouse_pos);

	void innerRectDragContinuation(int edge_mask, QPointF const& mouse_pos);

	void middleRectDragContinuation(int edge_mask, QPointF const& mouse_pos);

	void dragFinished();
	
	void recalcBoxesAndFit(RelativeMargins const& margins);
	
	void updatePresentationTransform(FitMode fit_mode);
	
	void forceNonNegativeHardMargins(QRectF& middle_rect) const;
	
	RelativeMargins calcHardMargins() const;
	
	void recalcOuterRect();
	
	AggregateSizeChanged commitHardMargins(RelativeMargins const& margins);
	
	void invalidateThumbnails(AggregateSizeChanged agg_size_changed);
	
	DraggableObject m_innerCorners[4];
	ObjectDragHandler m_innerCornerHandlers[4];
	DraggableObject m_innerEdges[4];
	ObjectDragHandler m_innerEdgeHandlers[4];

	DraggableObject m_middleCorners[4];
	ObjectDragHandler m_middleCornerHandlers[4];
	DraggableObject m_middleEdges[4];
	ObjectDragHandler m_middleEdgeHandlers[4];

	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;

	IntrusivePtr<Settings> m_ptrSettings;
	
	PageId const m_pageId;

	/**
	 * AffineImageTransform extracted from \p affine_transformed_image constructor parameter.
	 * This doesn't take image scaling as a result of MatchSizeMode::SCALE into account.
	 */
	imageproc::AffineImageTransform const m_unscaledAffineTransform;

	/**
	 * ContentBox in virtual image coordinates, prior to scaling by MatchSizeMode::SCALE.
	 */
	QRectF const m_unscaledContentRect;

	/**
	 * The point in affine_transformed_image.origImage() coordinates corresponding to
	 * the top-left corner of the content box. This point is obtained by reverse mapping
	 * of m_unscaledContentRect.topLeft() through m_unscaledAffineTransform.
	 *
	 * @note This member must follow both m_unscaledAffineTransform and m_unscaledContentRect.
	 */
	QPointF const m_affineImageContentTopLeft;
	
	/**
	 * Content box in virtual image coordinates, possibly scaled
	 * as a result of MatchSizeMode::SCALE.
	 */
	QRectF m_innerRect;
	
	/**
	 * \brief Content box + hard margins in virtual image coordinates.
	 *
	 * Hard margins are margins that will be there no matter what.
	 * Soft margins are those added to extend the page to match its
	 * size with other pages.
	 */
	QRectF m_middleRect;
	
	/**
	 * \brief Content box + hard + soft margins in virtual image coordinates.
	 *
	 * Hard margins are margins that will be there no matter what.
	 * Soft margins are those added to extend the page to match its
	 * size with other pages.
	 */
	QRectF m_outerRect;
	
	/**
	 * \brief Aggregate (max_width, max_height) hard page size in pixels.
	 *
	 * This one is for displaying purposes only.  It changes during
	 * dragging, and it may differ from what
	 * m_ptrSettings->getAggregateHardSize() would return.
	 *
	 * \see m_committedAggregateHardSize
	 */
	QSizeF m_aggregateHardSize;
	
	/**
	 * \brief Aggregate (max_width, max_height) hard page size in pixels.
	 *
	 * This one is supposed to be the cached version of what
	 * m_ptrSettings->getAggregateHardSize() would return.
	 *
	 * \see m_aggregateHardSize
	 */
	QSizeF m_committedAggregateHardSize;

	MatchSizeMode m_matchSizeMode;
	
	Alignment m_alignment;
	
	/**
	 * Some data saved at the beginning of a resizing operation.
	 */
	StateBeforeResizing m_beforeResizing;
	
	bool m_leftRightLinked;
	
	bool m_topBottomLinked;
};

} // namespace page_layout

#endif
