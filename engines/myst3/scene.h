/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MYST3_SCENE_H
#define MYST3_SCENE_H

#include "engines/myst3/rect.h"

#include "engines/myst3/gfx.h"

namespace Myst3 {

class Myst3Engine;
class SunSpot;

class Scene : public Window {
private:
	Myst3Engine *_vm;

	uint _mouseSpeed;

public:
	Scene(Myst3Engine *vm);

	/**
	 * Transform a point from screen coordinates to scaled window coordinates
	 */
	//TODO: Testing use of the base implementation in Window. Is an override needed here? If so, the implemented logic must be fixed. 
	//PointF scalePoint(const PointF &screen) const override;

	// Window API
	RectF getPosition() const override;
	RectF getOriginalPosition() const override;

	void updateCamera(const PointF &mouse);

	void updateMouseSpeed();

	void screenPosToDirection(const PointF &screen, float &pitch, float &heading) const;
	static Math::Vector3d directionToVector(float pitch, float heading);

	void drawSunspotFlare(const SunSpot &s);
	float distanceToZone(float spotHeading, float spotPitch, float spotRadius, float heading, float pitch);
};

} // end of namespace Myst3

#endif
