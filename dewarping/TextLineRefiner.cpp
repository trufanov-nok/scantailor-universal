/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "TextLineRefiner.h"
#include "VecNT.h"
#include "NumericTraits.h"
#include "DebugImages.h"
#include "imageproc/GrayImage.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/Sobel.h"
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <Qt>
#include <QtGlobal>
#include <QDebug>
#include <limits>
#include <algorithm>
#include <math.h>
#include <assert.h>

using namespace imageproc;

namespace dewarping
{

class TextLineRefiner::SnakeLength
{
public:
	explicit SnakeLength(Snake const& snake);

	float totalLength() const { return m_totalLength; }

	float avgSegmentLength() const { return m_avgSegmentLength; }

	float arcLengthAt(size_t node_idx) const { return m_integralLength[node_idx]; }

	float arcLengthFractionAt(size_t node_idx) const {
		return m_integralLength[node_idx] * m_rTotalLength;
	}

	float lengthFromTo(size_t from_node_idx, size_t to_node_idx) const {
		return m_integralLength[to_node_idx] - m_integralLength[from_node_idx];
	}
private:
	std::vector<float> m_integralLength;
	float m_totalLength;
	float m_rTotalLength; // Reciprocal of the above.
	float m_avgSegmentLength;
};


struct TextLineRefiner::FrenetFrame
{
	Vec2f unitTangent;
	Vec2f unitDownNormal;
};


class TextLineRefiner::Optimizer
{
public:
	Optimizer(Snake const& snake, Vec2f const& unit_down_vec, float factor);

	bool thicknessAdjustment(Snake& snake,
		std::function<float(QPointF const&)> const& top_attraction_force,
		std::function<float(QPointF const&)> const& bottom_attraction_force);

	bool tangentMovement(Snake& snake,
		std::function<float(QPointF const&)> const& top_attraction_force,
		std::function<float(QPointF const&)> const& bottom_attraction_force);

	bool normalMovement(Snake& snake,
		std::function<float(QPointF const&)> const& top_attraction_force,
		std::function<float(QPointF const&)> const& bottom_attraction_force);
private:
	static float calcExternalEnergy(
		std::function<float(QPointF const&)> const& top_attraction_force,
		std::function<float(QPointF const&)> const& bottom_attraction_force,
		SnakeNode const& node, Vec2f const down_normal);

	static float calcElasticityEnergy(
		SnakeNode const& node1, SnakeNode const& node2, float avg_dist);

	static float calcBendingEnergy(
		SnakeNode const& node, SnakeNode const& prev_node, SnakeNode const& prev_prev_node);

	static float const m_elasticityWeight;
	static float const m_bendingWeight;
	static float const m_topExternalWeight;
	static float const m_bottomExternalWeight;
	float const m_factor;
	SnakeLength m_snakeLength;
	std::vector<FrenetFrame> m_frenetFrames;
};


TextLineRefiner::TextLineRefiner(
	std::list<std::vector<QPointF> > const& polylines, Vec2f const& unit_down_vec)
:	m_unitDownVec(unit_down_vec)
{
	// Convert from polylines to snakes.
	for (std::vector<QPointF> const& polyline : polylines) {
		if (polyline.size() > 1) {
			m_snakes.push_back(makeSnake(polyline));
		}
	}
}

void
TextLineRefiner::refine(
	std::function<float(QPointF const&)> const& top_attraction_force,
	std::function<float(QPointF const&)> const& bottom_attraction_force,
	int const iterations, OnConvergence const on_convergence)
{
	for (Snake& snake : m_snakes) {
		evolveSnake(
			snake, top_attraction_force, bottom_attraction_force,
			iterations, on_convergence
		);
	}
}

std::list<std::vector<QPointF>>
TextLineRefiner::refinedPolylines() const
{
	std::list<std::vector<QPointF>> polylines;

	for (Snake const& snake : m_snakes) {
		polylines.push_back(std::vector<QPointF>());
		std::vector<QPointF>& polyline = polylines.back();

		for (SnakeNode const& node : snake.nodes) {
			polyline.push_back(node.center);
		}
	}

	return polylines;
}

QImage
TextLineRefiner::visualize(QImage const& background) const
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);

	QPen top_pen(QColor(0, 0, 255, 180));
	top_pen.setWidthF(2.0);

	QPen bottom_pen(QColor(255, 0, 0, 180));
	bottom_pen.setWidthF(2.0);

	QPen middle_pen(QColor(255, 0, 255, 180));
	middle_pen.setWidth(2.0);

	QBrush knot_brush(QColor(255, 255, 0, 180));
	painter.setBrush(knot_brush);

	QRectF knot_rect(0, 0, 7, 7);
	std::vector<FrenetFrame> frenet_frames;

	for (Snake const& snake : m_snakes) {
		SnakeLength const snake_length(snake);
		calcFrenetFrames(frenet_frames, snake, snake_length, m_unitDownVec);
		QVector<QPointF> top_polyline;
		QVector<QPointF> middle_polyline;
		QVector<QPointF> bottom_polyline;

		size_t const num_nodes = snake.nodes.size();
		for (size_t i = 0; i < num_nodes; ++i) {
			QPointF const mid(snake.nodes[i].center + QPointF(0.5, 0.5));
			QPointF const top(mid - snake.nodes[i].ribHalfLength * frenet_frames[i].unitDownNormal);
			QPointF const bottom(mid + snake.nodes[i].ribHalfLength * frenet_frames[i].unitDownNormal);
			top_polyline << top;
			middle_polyline << mid;
			bottom_polyline << bottom;
		}

		// Draw polylines.
		painter.setPen(top_pen);
		painter.drawPolyline(top_polyline);

		painter.setPen(bottom_pen);
		painter.drawPolyline(bottom_polyline);
#if 0
		painter.setPen(middle_pen);
		painter.drawPolyline(middle_polyline);
#endif

		// Draw knots.
		painter.setPen(Qt::NoPen);
		for (QPointF const& pt : middle_polyline) {
			knot_rect.moveCenter(pt);
			painter.drawEllipse(knot_rect);
		}
	}

	return canvas;
}

TextLineRefiner::Snake
TextLineRefiner::makeSnake(std::vector<QPointF> const& polyline)
{
	assert(polyline.size() > 1);

	float total_length = 0;

	size_t const polyline_size = polyline.size();
	for (size_t i = 1; i < polyline_size; ++i) {
		total_length += sqrt(Vec2f(polyline[i] - polyline[i - 1]).squaredNorm());
	}

	int const points_in_snake = std::min<size_t>(polyline.size(), 50);
	Snake snake;

	int points_inserted = 0;
	float base_t = 0;
	float next_insert_t = 0;
	for (size_t i = 1; i < polyline_size; ++i) {
		Vec2f const base(polyline[i - 1]);
		Vec2f const vec((polyline[i] - base));
		float const next_t = base_t + sqrt(vec.squaredNorm());
		
		while (next_t + 1e-3f >= next_insert_t) {
			float const fraction = (next_insert_t - base_t) / (next_t - base_t);
			SnakeNode node;
			node.center = base + fraction * vec;
			node.ribHalfLength = 4;
			snake.nodes.push_back(node);
			++points_inserted;
			next_insert_t = total_length * points_inserted / (points_in_snake - 1);
		}

		base_t = next_t;
	}
	
	return snake;
}

void
TextLineRefiner::calcFrenetFrames(
	std::vector<FrenetFrame>& frenet_frames, Snake const& snake,
	SnakeLength const& snake_length, Vec2f const& unit_down_vec)
{
	size_t const num_nodes = snake.nodes.size();
	frenet_frames.resize(num_nodes);
	
	if (num_nodes == 0) {
		return;
	} else if (num_nodes == 1) {
		frenet_frames[0].unitTangent = Vec2f();
		frenet_frames[0].unitDownNormal = Vec2f();
		return;
	}

	// First segment.
	Vec2f first_segment(snake.nodes[1].center - snake.nodes[0].center);
	float const first_segment_len = snake_length.arcLengthAt(1);
	if (first_segment_len > std::numeric_limits<float>::epsilon()) {
		first_segment /= first_segment_len;
		frenet_frames.front().unitTangent = first_segment;
	}

	// Segments between first and last, exclusive.
	Vec2f prev_segment(first_segment);
	for (size_t i = 1; i < num_nodes - 1; ++i) {
		Vec2f next_segment(snake.nodes[i + 1].center - snake.nodes[i].center);
		float const next_segment_len = snake_length.lengthFromTo(i, i + 1);
		if (next_segment_len > std::numeric_limits<float>::epsilon()) {
			next_segment /= next_segment_len;
		}

		Vec2f tangent_vec(0.5 * (prev_segment + next_segment));
		float const len = sqrt(tangent_vec.squaredNorm());
		if (len > std::numeric_limits<float>::epsilon()) {
			tangent_vec /= len;
		}
		frenet_frames[i].unitTangent = tangent_vec;

		prev_segment = next_segment;
	}

	// Last segments.
	Vec2f last_segment(snake.nodes[num_nodes - 1].center - snake.nodes[num_nodes - 2].center);
	float const last_segment_len = snake_length.lengthFromTo(num_nodes - 2, num_nodes - 1);
	if (last_segment_len > std::numeric_limits<float>::epsilon()) {
		last_segment /= last_segment_len;
		frenet_frames.back().unitTangent = last_segment;
	}

	// Calculate normals and make sure they point down.
	BOOST_FOREACH(FrenetFrame& frame, frenet_frames) {
		frame.unitDownNormal = Vec2f(frame.unitTangent[1], -frame.unitTangent[0]);
		if (frame.unitDownNormal.dot(unit_down_vec) < 0) {
			frame.unitDownNormal = -frame.unitDownNormal;
		}
	}
}

void
TextLineRefiner::evolveSnake(Snake& snake,
	std::function<float(QPointF const&)> const& top_attraction_force,
	std::function<float(QPointF const&)> const& bottom_attraction_force,
	int const iterations, OnConvergence const on_convergence)
{
	float factor = 1.0f;

	for (int i = 0; i < iterations; ++i) {
		Optimizer optimizer(snake, m_unitDownVec, factor);
		bool changed = false;
		changed |= optimizer.thicknessAdjustment(
			snake, top_attraction_force, bottom_attraction_force
		);
		changed |= optimizer.tangentMovement(
			snake, top_attraction_force, bottom_attraction_force
		);
		changed |= optimizer.normalMovement(
			snake, top_attraction_force, bottom_attraction_force
		);

		if (!changed) {
			if (on_convergence == ON_CONVERGENCE_STOP) {
				break;
			} else {
				factor *= 0.5f;
			}
		}
	}
}


/*============================ SnakeLength =============================*/

TextLineRefiner::SnakeLength::SnakeLength(Snake const& snake)
: m_integralLength(snake.nodes.size())
, m_totalLength()
, m_rTotalLength()
, m_avgSegmentLength()
{
	size_t const num_nodes = snake.nodes.size();
	float arc_length_accum = 0;
	for (size_t i = 1; i < num_nodes; ++i) { 
		Vec2f const vec(snake.nodes[i].center - snake.nodes[i - 1].center);
		arc_length_accum += sqrt(vec.squaredNorm());
		m_integralLength[i] = arc_length_accum;
	}
	m_totalLength = arc_length_accum;
	if (m_totalLength > std::numeric_limits<float>::epsilon()) {
		m_rTotalLength = 1.0f / m_totalLength;
	}
	if (num_nodes > 1) {
		m_avgSegmentLength = m_totalLength / (num_nodes - 1);
	}
}


/*=========================== Optimizer =============================*/

float const TextLineRefiner::Optimizer::m_elasticityWeight = 0.2f;
float const TextLineRefiner::Optimizer::m_bendingWeight = 7.0f;
float const TextLineRefiner::Optimizer::m_topExternalWeight = 0.3f;
float const TextLineRefiner::Optimizer::m_bottomExternalWeight = 0.3f;

TextLineRefiner::Optimizer::Optimizer(
	Snake const& snake, Vec2f const& unit_down_vec, float factor)
:	m_factor(factor)
,	m_snakeLength(snake)
{
	calcFrenetFrames(m_frenetFrames, snake, m_snakeLength, unit_down_vec);
}

bool
TextLineRefiner::Optimizer::thicknessAdjustment(Snake& snake,
	std::function<float(QPointF const&)> const& top_attraction_force,
	std::function<float(QPointF const&)> const& bottom_attraction_force)
{
	size_t const num_nodes = snake.nodes.size();

	float const rib_adjustments[] = { 0.0f * m_factor, 0.5f * m_factor, -0.5f * m_factor };
	enum { NUM_RIB_ADJUSTMENTS = sizeof(rib_adjustments)/sizeof(rib_adjustments[0]) };

	int best_i = 0;
	int best_j = 0;
	float best_cost = NumericTraits<float>::max();
	for (int i = 0; i < NUM_RIB_ADJUSTMENTS; ++i) {
		float const head_rib = snake.nodes.front().ribHalfLength + rib_adjustments[i];
		if (head_rib <= std::numeric_limits<float>::epsilon()) {
			continue;
		}

		for (int j = 0; j < NUM_RIB_ADJUSTMENTS; ++j) {
			float const tail_rib = snake.nodes.back().ribHalfLength + rib_adjustments[j];
			if (tail_rib <= std::numeric_limits<float>::epsilon()) {
				continue;
			}
			
			float cost = 0;
			for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
				float const t = m_snakeLength.arcLengthFractionAt(node_idx);
				float const rib = head_rib + t * (tail_rib - head_rib);
				Vec2f const down_normal(m_frenetFrames[node_idx].unitDownNormal);

				SnakeNode node(snake.nodes[node_idx]);
				node.ribHalfLength = rib;
				cost += calcExternalEnergy(
					top_attraction_force, bottom_attraction_force, node, down_normal
				);
			}
			if (cost < best_cost) {
				best_cost = cost;
				best_i = i;
				best_j = j;
			}
		}
	}
	float const head_rib = snake.nodes.front().ribHalfLength + rib_adjustments[best_i];
	float const tail_rib = snake.nodes.back().ribHalfLength + rib_adjustments[best_j];
	for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
		float const t = m_snakeLength.arcLengthFractionAt(node_idx);
		snake.nodes[node_idx].ribHalfLength = head_rib + t * (tail_rib - head_rib);
		// Note that we need to recalculate inner ribs even if outer ribs
		// didn't change, as movement of ribs in tangent direction affects
		// interpolation.
	}
	
	return rib_adjustments[best_i] != 0 || rib_adjustments[best_j] != 0;
}

bool
TextLineRefiner::Optimizer::tangentMovement(Snake& snake,
	std::function<float(QPointF const&)> const& top_attraction_force,
	std::function<float(QPointF const&)> const& bottom_attraction_force)
{
	size_t const num_nodes = snake.nodes.size();
	if (num_nodes < 3) {
		return false;
	}

	float const tangent_movements[] = { 0.0f * m_factor, 1.0f * m_factor, -1.0f * m_factor };
	enum { NUM_TANGENT_MOVEMENTS = sizeof(tangent_movements)/sizeof(tangent_movements[0]) };

	std::vector<uint32_t> paths;
	std::vector<uint32_t> new_paths;
	std::vector<Step> step_storage;

	// Note that we don't move the first and the last node in tangent direction.
	paths.push_back(step_storage.size());
	step_storage.push_back(Step());
	step_storage.back().prevStepIdx = ~uint32_t(0);
	step_storage.back().node = snake.nodes.front();
	step_storage.back().pathCost = 0;

	for (size_t node_idx = 1; node_idx < num_nodes - 1; ++node_idx) {
		Vec2f const initial_pos(snake.nodes[node_idx].center);
		float const rib = snake.nodes[node_idx].ribHalfLength;
		Vec2f const unit_tangent(m_frenetFrames[node_idx].unitTangent);
		Vec2f const down_normal(m_frenetFrames[node_idx].unitDownNormal);

		for (int i = 0; i < NUM_TANGENT_MOVEMENTS; ++i) {
			Step step;
			step.prevStepIdx = ~uint32_t(0);
			step.node.center = initial_pos + tangent_movements[i] * unit_tangent;
			step.node.ribHalfLength = rib;
			step.pathCost = NumericTraits<float>::max();

			float base_cost = calcExternalEnergy(
				top_attraction_force, bottom_attraction_force, step.node, down_normal
			);

			if (node_idx == num_nodes - 2) {
				// Take into account the distance to the last node as well.
				base_cost += calcElasticityEnergy(
					step.node, snake.nodes.back(), m_snakeLength.avgSegmentLength()
				);
			}

			// Now find the best step for the previous node to combine with.
			BOOST_FOREACH(uint32_t prev_step_idx, paths) {
				Step const& prev_step = step_storage[prev_step_idx];
				float const cost = base_cost + prev_step.pathCost +
					calcElasticityEnergy(step.node, prev_step.node, m_snakeLength.avgSegmentLength());

				if (cost < step.pathCost) {
					step.pathCost = cost;
					step.prevStepIdx = prev_step_idx;
				}
			}
			assert(step.prevStepIdx != ~uint32_t(0));
			new_paths.push_back(step_storage.size());
			step_storage.push_back(step);
		}
		assert(!new_paths.empty());
		paths.swap(new_paths);
		new_paths.clear();
	}
	
	// Find the best overall path.
	uint32_t best_path_idx = ~uint32_t(0);
	float best_cost = NumericTraits<float>::max();
	BOOST_FOREACH(uint32_t last_step_idx, paths) {
		Step const& step = step_storage[last_step_idx];
		if (step.pathCost < best_cost) {
			best_cost = step.pathCost;
			best_path_idx = last_step_idx;
		}
	}

	// Having found the best path, convert it back to a snake.
	float max_sqdist = 0;
	uint32_t step_idx = best_path_idx;
	for (int node_idx = num_nodes - 2; node_idx > 0; --node_idx) {
		assert(step_idx != ~uint32_t(0));
		Step const& step = step_storage[step_idx];
		SnakeNode& node = snake.nodes[node_idx];
		
		float const sqdist = (node.center - step.node.center).squaredNorm();
		max_sqdist = std::max<float>(max_sqdist, sqdist);

		node = step.node;
		step_idx = step.prevStepIdx;
	}

	return max_sqdist > std::numeric_limits<float>::epsilon();
}

bool
TextLineRefiner::Optimizer::normalMovement(Snake& snake,
	std::function<float(QPointF const&)> const& top_attraction_force,
	std::function<float(QPointF const&)> const& bottom_attraction_force)
{
	size_t const num_nodes = snake.nodes.size();
	if (num_nodes < 3) {
		return false;
	}

	float const normal_movements[] = { 0.0f * m_factor, 1.0f * m_factor, -1.0f * m_factor };
	enum { NUM_NORMAL_MOVEMENTS = sizeof(normal_movements)/sizeof(normal_movements[0]) };

	std::vector<uint32_t> paths;
	std::vector<uint32_t> new_paths;
	std::vector<Step> step_storage;

	// The first two nodes pose a problem for us.  These nodes don't have two predecessors,
	// and therefore we can't take bending into the account.  We could take the followers
	// instead of the ancestors, but then this follower is going to move itself, making
	// our calculations less accurate.  The proper solution is to provide not N but N*N
	// paths to the 3rd node, each path corresponding to a combination of movement of
	// the first and the second node.  That's the approach we are taking here.
	for (int i = 0; i < NUM_NORMAL_MOVEMENTS; ++i) {
		uint32_t const prev_step_idx = step_storage.size();
		{
			// Movements of the first node.
			Vec2f const down_normal(m_frenetFrames[0].unitDownNormal);
			Step step;
			step.node.center = snake.nodes[0].center + normal_movements[i] * down_normal;
			step.node.ribHalfLength = snake.nodes[0].ribHalfLength;
			step.prevStepIdx = ~uint32_t(0);
			step.pathCost = calcExternalEnergy(
				top_attraction_force, bottom_attraction_force, step.node, down_normal
			);

			step_storage.push_back(step);
		}
		
		for (int j = 0; j < NUM_NORMAL_MOVEMENTS; ++j) {
			// Movements of the second node.
			Vec2f const down_normal(m_frenetFrames[1].unitDownNormal);
			
			Step step;
			step.node.center = snake.nodes[1].center + normal_movements[j] * down_normal;
			step.node.ribHalfLength = snake.nodes[1].ribHalfLength;
			step.prevStepIdx = prev_step_idx;
			step.pathCost = step_storage[prev_step_idx].pathCost + calcExternalEnergy(
				top_attraction_force, bottom_attraction_force, step.node, down_normal
			);

			paths.push_back(step_storage.size());
			step_storage.push_back(step);
		}
	}

	for (size_t node_idx = 2; node_idx < num_nodes; ++node_idx) {
		SnakeNode const& node = snake.nodes[node_idx];
		Vec2f const down_normal(m_frenetFrames[node_idx].unitDownNormal);

		for (int i = 0; i < NUM_NORMAL_MOVEMENTS; ++i) {
			Step step;
			step.prevStepIdx = ~uint32_t(0);
			step.node.center = node.center + normal_movements[i] * down_normal;
			step.node.ribHalfLength = node.ribHalfLength;
			step.pathCost = NumericTraits<float>::max();

			float const base_cost = calcExternalEnergy(
				top_attraction_force, bottom_attraction_force, step.node, down_normal
			);

			// Now find the best step for the previous node to combine with.
			BOOST_FOREACH(uint32_t prev_step_idx, paths) {
				Step const& prev_step = step_storage[prev_step_idx];
				Step const& prev_prev_step = step_storage[prev_step.prevStepIdx];

				float const cost = base_cost + prev_step.pathCost +
					calcBendingEnergy(step.node, prev_step.node, prev_prev_step.node);

				if (cost < step.pathCost) {
					step.pathCost = cost;
					step.prevStepIdx = prev_step_idx;
				}
			}
			assert(step.prevStepIdx != ~uint32_t(0));
			new_paths.push_back(step_storage.size());
			step_storage.push_back(step);
		}
		assert(!new_paths.empty());
		paths.swap(new_paths);
		new_paths.clear();
	}
	
	// Find the best overall path.
	uint32_t best_path_idx = ~uint32_t(0);
	float best_cost = NumericTraits<float>::max();
	BOOST_FOREACH(uint32_t last_step_idx, paths) {
		Step const& step = step_storage[last_step_idx];
		if (step.pathCost < best_cost) {
			best_cost = step.pathCost;
			best_path_idx = last_step_idx;
		}
	}

	// Having found the best path, convert it back to a snake.
	float max_sqdist = 0;
	uint32_t step_idx = best_path_idx;
	for (int node_idx = num_nodes - 1; node_idx >= 0; --node_idx) {
		assert(step_idx != ~uint32_t(0));
		Step const& step = step_storage[step_idx];
		SnakeNode& node = snake.nodes[node_idx];
		
		float const sqdist = (node.center - step.node.center).squaredNorm();
		max_sqdist = std::max<float>(max_sqdist, sqdist);

		node = step.node;
		step_idx = step.prevStepIdx;
	}

	return max_sqdist > std::numeric_limits<float>::epsilon();
}

float
TextLineRefiner::Optimizer::calcExternalEnergy(
	std::function<float(QPointF const&)> const& top_attraction_force,
	std::function<float(QPointF const&)> const& bottom_attraction_force,
	SnakeNode const& node, Vec2f const down_normal)
{
	Vec2f const top(node.center - node.ribHalfLength * down_normal);
	Vec2f const bottom(node.center + node.ribHalfLength * down_normal);

	float const top_force = m_topExternalWeight * top_attraction_force(top);
	float const bottom_force = m_bottomExternalWeight * bottom_attraction_force(bottom);

	// We mimimize rather than maximize the energy, therefore we negate
	// force and minimize that.
	return -std::abs(top_force * bottom_force);
}

float
TextLineRefiner::Optimizer::calcElasticityEnergy(
	SnakeNode const& node1, SnakeNode const& node2, float avg_dist)
{
	Vec2f const vec(node1.center - node2.center);
	float const vec_len = sqrt(vec.squaredNorm());

	if (vec_len < 1.0f) {
		return 1000.0f; // Penalty for moving too close to another node.
	}

	float const dist_diff = fabs(avg_dist - vec_len);
	return m_elasticityWeight * (dist_diff / avg_dist);
}

float
TextLineRefiner::Optimizer::calcBendingEnergy(
	SnakeNode const& node, SnakeNode const& prev_node, SnakeNode const& prev_prev_node)
{
	Vec2f const vec(node.center - prev_node.center);
	float const vec_len = sqrt(vec.squaredNorm());

	if (vec_len < 1.0f) {
		return 1000.0f; // Penalty for moving too close to another node.
	}

	Vec2f const prev_vec(prev_node.center - prev_prev_node.center);
	float const prev_vec_len = sqrt(prev_vec.squaredNorm());
	if (prev_vec_len < 1.0f) {
		return 1000.0f; // Penalty for moving too close to another node.
	}

	Vec2f const bend_vec(vec / vec_len - prev_vec / prev_vec_len);
	return m_bendingWeight * bend_vec.squaredNorm();
}

} // namespace dewarping
