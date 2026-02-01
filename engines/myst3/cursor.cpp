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

#include "graphics/cursorman.h"
#include "graphics/surface.h"

#include "image/bmp.h"

namespace Myst3 {

Cursor::Cursor(Myst3Engine *vm) :
	_vm(vm),
	_position(vm->_scene->getCenter()),
	_currentCursorID(0),
	_hideLevel(0),
	_lockedAtCenter(false) {

	// Load available cursors
	loadAvailableCursors();

	// Push initial cursor onto CursorMan stack
	CursorMan.pushCursor(nullptr, 0, 0, 0, 0, 0);
	CursorMan.showMouse(true);

	// Set default cursor
	changeCursor(8);
	g_system->warpMouse(_position.x, _position.y);
}

void Cursor::loadAvailableCursors() {
	assert(_surfaces.empty());
	assert(_textures.empty());

	TextureLoader textureLoader(*_vm->_gfx);

	// Load available cursors
	for (uint i = 0; i < ARRAYSIZE(availableCursors); i++) {
		// Check if a cursor sharing the same image has already been loaded
		if (_surfaces.contains(availableCursors[i].nodeID)) continue;

		// Load the cursor bitmap
		ResourceDescription cursorDesc = _vm->_resourceLoader->getRawData("GLOB", availableCursors[i].nodeID);
		if (!cursorDesc.isValid())
			error("Cursor %d does not exist", availableCursors[i].nodeID);

		// Load as surface for hardware cursor (CursorMan)
		Graphics::Surface *cursorSurface = textureLoader.loadSurface(cursorDesc, TextureLoader::kImageFormatBMP);
		_surfaces.setVal(availableCursors[i].nodeID, cursorSurface);

		// Load as GPU texture for manual drawing when locked at center
		Texture *cursorTexture = textureLoader.load(cursorDesc, TextureLoader::kImageFormatBMP);
		_textures.setVal(availableCursors[i].nodeID, cursorTexture);

		debugC(kDebugModding, "Cursor loaded - id: %d, size: %dx%d", availableCursors[i].nodeID, cursorSurface->w, cursorSurface->h);
	}
}

Cursor::~Cursor() {
	// Pop cursor from CursorMan stack
	CursorMan.popCursor();

	// Free cursor surfaces
	for (SurfaceMap::iterator it = _surfaces.begin(); it != _surfaces.end(); it++) {
		it->_value->free();
		delete it->_value;
	}

	// Free cursor textures
	for (TextureMap::iterator it = _textures.begin(); it != _textures.end(); it++) {
		delete it->_value;
	}
}

void Cursor::changeCursor(uint32 index) {
	if (index >= ARRAYSIZE(availableCursors))
		return;

	if (_vm->getPlatform() == Common::kPlatformXbox) {
		// The cursor is hidden when it is not hovering hotspots
		if ((index == 0 || index == 8) && _vm->_state->getViewType() != kCube)
			index = 12;
	}

	if (_currentCursorID == index)
		return;

	_currentCursorID = index;

	// Update hardware cursor (only used when not locked at center)
	CursorData cursor(_currentCursorID);
	Graphics::Surface *surface = _surfaces[cursor.nodeID];
	if (!surface) {
		error("No surface for cursor with id %d", cursor.nodeID);
	}

	// Use 0x00000000 (transparent black) as the keycolor for RGBA surfaces
	// The BMP loader uses green (0,255,0) as transparency which gets converted to alpha=0
	uint32 keycolor = surface->format.ARGBToColor(0, 0, 0, 0);

	CursorMan.replaceCursor(*surface, cursor.hotspotX, cursor.hotspotY, keycolor, false);
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
		// Locking - hide system cursor, we'll draw manually at center
		_position = center;
		CursorMan.showMouse(false);
	} else {
		// Unlocking - show system cursor, warp to center
		g_system->warpMouse(center.x, center.y);
		bool shouldBeVisible = !_hideLevel && !_vm->_state->getCursorHidden() && !_vm->_state->getCursorLocked();
		CursorMan.showMouse(shouldBeVisible);
	}
}

void Cursor::updatePosition(const Point &mouse) {
	if (!_lockedAtCenter) {
		_position = mouse;
	} else {
		_position = _vm->_scene->getCenter();
	}
}

void Cursor::draw() {
	if (_lockedAtCenter) {
		// When locked at center, draw cursor manually using GPU texture
		if (!isVisible())
			return;

		CursorData cursor(_currentCursorID);
		Point cursorHotspot = cursor.getHotspot();
		Texture *texture = _textures[cursor.nodeID];
		if (!texture) {
			error("No texture for cursor with id %d", cursor.nodeID);
		}

		// Draw at center of screen
		Point center = _vm->_scene->getCenter();
		Rect cursorRect = texture->size();
		cursorRect.translate(center.x - cursorHotspot.x, center.y - cursorHotspot.y);

		float transparency = 1.0f;
		int32 varTransparency = _vm->_state->getCursorTransparency();
		if (varTransparency == 0) {
			if (varTransparency >= 0)
				transparency = varTransparency / 100.0f;
			else
				transparency = getTransparencyForId(_currentCursorID);
		}

		Rect textureRect(texture->width, texture->height);
		_vm->_gfx->drawTexturedRect2D(cursorRect, textureRect, texture, transparency);
	} else {
		// When not locked, system cursor handles drawing
		// Just ensure visibility is correct
		bool shouldBeVisible = !_hideLevel && !_vm->_state->getCursorHidden() && !_vm->_state->getCursorLocked();
		CursorMan.showMouse(shouldBeVisible);
	}
}

void Cursor::setVisible(bool show) {
	if (show)
		_hideLevel = MAX<int32>(0, --_hideLevel);
	else
		_hideLevel++;

	// Update cursor visibility (only affects system cursor when not locked)
	if (!_lockedAtCenter) {
		bool shouldBeVisible = !_hideLevel && !_vm->_state->getCursorHidden() && !_vm->_state->getCursorLocked();
		CursorMan.showMouse(shouldBeVisible);
	}
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
