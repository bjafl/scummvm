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

#include "engines/myst3/archive.h"
#include "engines/myst3/cursor.h"
#include "engines/myst3/gfx.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/resource_loader.h"
#include "engines/myst3/scene.h"
#include "engines/myst3/state.h"

#include "graphics/surface.h"

#include "image/bmp.h"

namespace Myst3 {

Cursor::Cursor(Myst3Engine *vm) :
	_vm(vm),
	_position(vm->_scene->getCenter()),
	_currentCursorID(0),
	_hideLevel(0),
	_lockedAtCenter(false) {

	// The cursor is manually scaled
	_scaled = false;
	_isConstrainedToWindow = false;

	// Load available cursors
	loadAvailableCursors();

	// Set default cursor
	changeCursor(8);
	g_system->warpMouse(_position.x, _position.y);
}

void Cursor::loadAvailableCursors() {
	assert(_textures.empty());

	TextureLoader textureLoader(*_vm->_gfx);

	// Load available cursors
	for (uint i = 0; i < ARRAYSIZE(availableCursors); i++) {
		// Check if a cursor sharing the same texture has already been loaded
		if (_textures.contains(availableCursors[i].nodeID)) continue;

		// Load the cursor bitmap
		ResourceDescription cursorDesc = _vm->_resourceLoader->getRawData("GLOB", availableCursors[i].nodeID);
		if (!cursorDesc.isValid())
			error("Cursor %d does not exist", availableCursors[i].nodeID);

		// Create and store the texture
		Texture *cursorTexture = textureLoader.load(cursorDesc, TextureLoader::kImageFormatBMP);
		_textures.setVal(availableCursors[i].nodeID, cursorTexture);

		debugC(kDebugModding, "Cursor loaded - id: %d", availableCursors[i].nodeID);
	}
}

Cursor::~Cursor() {
	// Free cursors textures
	for (TextureMap::iterator it = _textures.begin(); it != _textures.end(); it++) {
		delete it->_value;
	}
}

void Cursor::changeCursor(uint32 index) {
	if (index >= ARRAYSIZE(availableCursors) || index < 0)
		return;

	if (_vm->getPlatform() == Common::kPlatformXbox) {
		// The cursor is hidden when it is not hovering hotspots
		if ((index == 0 || index == 8) && _vm->_state->getViewType() != kCube)
			index = 12;
	}

	_currentCursorID = index;
}

float Cursor::getTransparencyForId(uint32 cursorId) {
	assert(cursorId < ARRAYSIZE(availableCursors));
	if (_vm->getPlatform() == Common::kPlatformXbox) {
		return availableCursors[cursorId].transparencyXbox;
	} else {
		return availableCursors[cursorId].transparency;
	}
}

void Cursor::lockPosition(bool lock) {
	if (_lockedAtCenter == lock)
		return;

	_lockedAtCenter = lock;

	g_system->lockMouse(lock);

	Point center = _vm->_scene->getCenter();
	if (_lockedAtCenter) {
		// Locking, just move the cursor at the center of the screen
		_position = center;
	} else {
		// Unlocking, warp the actual mouse position to the cursor
		g_system->warpMouse(center.x, center.y);
	}
}

void Cursor::updatePosition(const Point &mouse) {
	if (!_lockedAtCenter) {
		_position = mouse;
	} else {
		_position = _vm->_scene->getCenter();
	}
}

// Point Cursor::getPosition(bool scaled) {
// 	if (scaled) {
// 		Rect viewport = _vm->_gfx->viewport();

// 		// The rest of the engine expects 640x480 coordinates
// 		Point scaledPosition = _position;
// 		scaledPosition.x -= viewport.left;
// 		scaledPosition.y -= viewport.top;
// 		scaledPosition.x = CLIP<int16>(scaledPosition.x, 0, viewport.width());
// 		scaledPosition.y = CLIP<int16>(scaledPosition.y, 0, viewport.height());
// 		scaledPosition.x *= Renderer::kOriginalWidth / (float) viewport.width();
// 		scaledPosition.y *= Renderer::kOriginalHeight / (float) viewport.height();

// 		return scaledPosition;
// 	} else {
// 		return _position;
// 	}
// }

void Cursor::draw() {
	assert(_currentCursorID < ARRAYSIZE(availableCursors));

	//const CursorData &cursor = availableCursors[_currentCursorID];
	CursorData cursor(_currentCursorID);
	Point cursorHotspot = cursor.getHotspot();
	Rect cursorSize = cursor.size();
	Texture *texture = _textures[cursor.nodeID];
	if (!texture) {
		error("No texture for cursor with id %d", cursor.nodeID);
	}

	// Rect where to draw the cursor
	Rect viewport = _vm->_gfx->viewport();
	PointF scale = _vm->_gfx->getScale();
	scale = scale * (cursorSize.width() / (float)texture->width);
	
	Rect cursorRect = texture->size();
	cursorRect.setWidth(cursorRect.width() * scale.x);
	cursorRect.setHeight(cursorRect.height() * scale.y);
	cursorRect.translate(_position.x - cursorHotspot.x * scale.x, _position.y - cursorHotspot.y * scale.y);

	float transparency = 1.0f;

	int32 varTransparency = _vm->_state->getCursorTransparency();
	if (_lockedAtCenter || varTransparency == 0) {
		if (varTransparency >= 0)
			transparency = varTransparency / 100.0f;
		else
			transparency = getTransparencyForId(_currentCursorID);
	}

	// _vm->_gfx->setViewport(viewport, false);
	Rect textureRect(texture->width, texture->height);
	_vm->_gfx->drawTexturedRect2D(cursorRect, textureRect, texture, transparency);
    debugC(kDebugUi, "Cursor drawTexturedRect2D - screen [%dx%d], texture [%dx%d]", cursorRect.width(), cursorRect.height(), textureRect.width(), textureRect.height());
}

void Cursor::setVisible(bool show) {
	if (show)
		_hideLevel = MAX<int32>(0, --_hideLevel);
	else
		_hideLevel++;
}

bool Cursor::isVisible() {
	return !_hideLevel && !_vm->_state->getCursorHidden() && !_vm->_state->getCursorLocked();
}

void Cursor::getDirection(float &pitch, float &heading) {
	if (_lockedAtCenter) {
		pitch = _vm->_state->getLookAtPitch();
		heading = _vm->_state->getLookAtHeading();
	} else {
		_vm->_scene->screenPosToDirection(_position, pitch, heading);
	}
}

} // End of namespace Myst3
