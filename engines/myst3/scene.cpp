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

#include "common/config-manager.h"

#include "engines/myst3/gfx.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/node.h"
#include "engines/myst3/scene.h"
#include "engines/myst3/state.h"

#include "math/utils.h"
#include "math/vector2d.h"

namespace Myst3 {

Scene::Scene(Myst3Engine *vm) : Window(),
								_vm(vm),
								_mouseSpeed(50) {
	updateMouseSpeed();
}

void Scene::updateCamera(const Point &mouse) {
	float pitch = _vm->_state->getLookAtPitch();
	float heading = _vm->_state->getLookAtHeading();

	if (!_vm->_state->getCursorLocked()) {
		float speed = 25 / (float)(200 - _mouseSpeed);

		// Adjust the speed according to the resolution
		float scale = _vm->_gfx->getScale().x;
		speed /= scale;

		if (ConfMan.getBool("mouse_inverted")) {
			pitch += mouse.y * speed;
		} else {
			pitch -= mouse.y * speed;
		}
		heading += mouse.x * speed;
	}

	// Keep heading within allowed values
	if (_vm->_state->isCameraLimited()) {
		float minHeading = _vm->_state->getMinHeading();
		float maxHeading = _vm->_state->getMaxHeading();

		if (minHeading < maxHeading) {
			heading = CLIP(heading, minHeading, maxHeading);
		} else {
			if (heading < minHeading && heading > maxHeading) {
				uint distToMin = (uint)ABS(heading - minHeading);
				uint distToMax = (uint)ABS(heading - maxHeading);
				if (distToMin > distToMax)
					heading = maxHeading;
				else
					heading = minHeading;
			}
		}
	}

	// Keep heading in 0..360 range
	if (heading > 360.0f)
		heading -= 360.0f;
	else if (heading < 0.0f)
		heading += 360.0f;

	// Keep pitch within allowed values
	float minPitch = _vm->_state->getCameraMinPitch();
	float maxPitch = _vm->_state->getCameraMaxPitch();

	if (_vm->_state->isCameraLimited()) {
		minPitch = _vm->_state->getMinPitch();
		maxPitch = _vm->_state->getMaxPitch();
	}

	pitch = CLIP(pitch, minPitch, maxPitch);

	_vm->_state->lookAt(pitch, heading);
	_vm->_state->setCameraPitch((int32)pitch);
	_vm->_state->setCameraHeading((int32)heading);
}

void Scene::drawSunspotFlare(const SunSpot &s) {
	Rect frame = _vm->_gfx->frameViewport();

	uint8 a = (uint8)(s.intensity * s.radius);
	uint8 r = (s.color >> 16) & 0xFF;
	uint8 g = (s.color >> 8) & 0xFF;
	uint8 b = (s.color >> 0) & 0xFF;

	_vm->_gfx->selectTargetWindow(this, false, true);
	_vm->_gfx->drawRect2D(frame, a, r, g, b);
}

Math::Vector3d Scene::directionToVector(float pitch, float heading) {
	Math::Vector3d v;

	float radHeading = Math::deg2rad(heading);
	float radPitch = Math::deg2rad(pitch);

	v.setValue(0, cos(radPitch) * cos(radHeading));
	v.setValue(1, sin(radPitch));
	v.setValue(2, cos(radPitch) * sin(radHeading));

	return v;
}

float Scene::distanceToZone(float spotHeading, float spotPitch, float spotRadius, float heading, float pitch) {
	Math::Vector3d vLookAt = directionToVector(pitch, heading);
	Math::Vector3d vSun = directionToVector(spotPitch, spotHeading);
	float dotProduct = Math::Vector3d::dotProduct(vLookAt, -vSun);

	float distance = (0.05 * spotRadius - (dotProduct + 1.0) * 90) / (0.05 * spotRadius);
	return CLIP<float>(distance, 0.0, 1.0);
}

void Scene::updateMouseSpeed() {
	_mouseSpeed = ConfMan.getInt("mouse_speed");
}

Rect Scene::getPosition() const {
	ViewType viewType = _vm->_state->getViewType();
	Rect frame = viewType == kMenu ? _vm->_gfx->origAspectRatioViewport() : _vm->_gfx->frameViewport();
	debugC(kDebugGraphics, "Scene (type: %d) frame x1,y1,x2,y2: %d,%d,%d,%d (WxH: %dx%d)", viewType, frame.top, frame.left, frame.bottom, frame.right, frame.width(), frame.height());
	return frame;
}

Rect Scene::getOriginalPosition() const {
	Rect originalPosition;

	if (_vm->_state->getViewType() != kMenu) {
		originalPosition = Rect(Renderer::kOriginalWidth, Renderer::kFrameHeight);
		originalPosition.translate(0, Renderer::kTopBorderHeight);
	} else {
		originalPosition = Rect(Renderer::kOriginalWidth, Renderer::kOriginalHeight);
	}

	return originalPosition;
}

void Scene::screenPosToDirection(const Point &screen, float &pitch, float &heading) const {
	// Rect frame = getPosition();
	Rect frame = _vm->_gfx->frameViewport();

	// Screen coords to window coords
	Point pos = screenPosToWindowPos(screen);

	// Window coords to normalized coords
	Math::Vector4d in;
	in.x() = pos.x * 2 / (float)frame.width() - 1.0;
	in.y() = 1.0 - pos.y * 2 / (float)frame.height();
	in.z() = 1.0;
	in.w() = 1.0;

	// Normalized coords to direction
	Math::Matrix4 A = _vm->_gfx->getMvpMatrix();
	A.inverse();
	Math::Vector4d out = A.transform(in);

	Math::Vector3d direction(out.x(), out.y(), out.z());
	direction.normalize();

	// 3D coords to polar coords
	Math::Vector2d horizontalProjection = Math::Vector2d(direction.x(), direction.z());
	horizontalProjection.normalize();

	pitch = 90 - Math::Angle::arcCosine(direction.y()).getDegrees();
	heading = Math::Angle::arcCosine(horizontalProjection.getY()).getDegrees();

	if (horizontalProjection.getX() > 0.0)
		heading = 360 - heading;
}

// TODO: below ported from residualvm. Check if it may / should be used, or remove. Compare with func in base class..
// Point Scene::scalePoint(const Point &screen) const {
// 	Rect viewport;
// 	Rect originalSize;
// 	if (_vm->_state->getViewType() == kMenu) {
// 		viewport = _vm->_gfx->viewport(); // TODO?menuViewport();
// 		originalSize = Rect(Renderer::kOriginalWidth, Renderer::kOriginalHeight);
// 	} else {
// 		viewport = _vm->_gfx->frameViewport();
// 		originalSize = Rect(Renderer::kOriginalWidth, Renderer::kFrameHeight);
// 	}

// 	Point scaledPosition = screen;
// 	scaledPosition.x -= viewport.left;
// 	scaledPosition.y -= viewport.top;
// 	scaledPosition.x = CLIP<int16>(scaledPosition.x, 0, viewport.width());
// 	scaledPosition.y = CLIP<int16>(scaledPosition.y, 0, viewport.height());

// 	scaledPosition.x *= originalSize.width() / (float)viewport.width();
// 	scaledPosition.y *= originalSize.height() / (float)viewport.height();

// 	return scaledPosition;
// }

} // end of namespace Myst3
