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

#ifndef DEWARPING_TEXT_LINE_REFINER_H_
#define DEWARPING_TEXT_LINE_REFINER_H_

#include "dewarping_config.h"
#include "VecNT.h"
#include <QPointF>
#include <QLineF>
#include <functional>
#include <vector>
#include <list>
#include <stdint.h>

class QImage;

namespace dewarping
{

class DEWARPING_EXPORT TextLineRefiner
{
	// Member-wise copying is OK.
public:
	enum OnConvergence { ON_CONVERGENCE_STOP, ON_CONVERGENCE_GO_FINER };

	TextLineRefiner(std::list<std::vector<QPointF> > const& polylines,
		Vec2f const& unit_down_vec);

	void refine(
		std::function<float(QPointF const&)> const& top_attraction_force,
		std::function<float(QPointF const&)> const& bottom_attraction_force,
		int iterations, OnConvergence on_convergence);

	std::list<std::vector<QPointF>> refinedPolylines() const;

	QImage visualize(QImage const& background) const;
private:
	class SnakeLength;
	struct FrenetFrame;
	class Optimizer;

	struct SnakeNode
	{
		Vec2f center;
		float ribHalfLength;
	};

	struct Snake
	{
		std::vector<SnakeNode> nodes;
	};

	struct Step
	{
		SnakeNode node;
		uint32_t prevStepIdx;
		float pathCost;
	};

	static Snake makeSnake(std::vector<QPointF> const& polyline);

	static void calcFrenetFrames(
		std::vector<FrenetFrame>& frenet_frames, Snake const& snake,
		SnakeLength const& snake_length, Vec2f const& unit_down_vec);

	void evolveSnake(Snake& snake,
		std::function<float(QPointF const&)> const& top_attraction_force,
		std::function<float(QPointF const&)> const& bottom_attraction_force,
		int iterations, OnConvergence on_convergence);
	
	Vec2f m_unitDownVec;
	std::vector<Snake> m_snakes;
};

} // namespace dewarping

#endif
