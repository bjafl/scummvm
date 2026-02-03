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

#ifndef CURSOR_H_
#define CURSOR_H_

#include "common/hashmap.h"

#include "engines/myst3/gfx.h"
#include "engines/myst3/rect.h"
#include "graphics/surface.h"

namespace Myst3 {

class Myst3Engine;
class Texture;

struct CursorDataStruct{
	uint32 nodeID;
	uint16 width;
	uint16 height;
	uint16 hotspotX;
	uint16 hotspotY;
	float transparency;
	float transparencyXbox;
};

static const CursorDataStruct availableCursors[] = {
	{1000, 16, 16, 8, 8, 0.25f, 0.00f}, // Default cursor
	{1001, 16, 16, 8, 8, 0.50f, 0.50f}, // On top of inventory item
	{1002, 16, 16, 8, 8, 0.50f, 0.50f}, // Drag cursor
	{1003, 16, 16, 1, 5, 0.50f, 0.50f},
	{1004, 16, 16, 14, 5, 0.50f, 0.50f},
	{1005, 24, 24, 16, 14, 0.50f, 0.50f},
	{1006, 24, 24, 16, 14, 0.50f, 0.50f},
	{1007, 16, 16, 8, 8, 0.55f, 0.55f},
	{1000, 16, 16, 8, 8, 0.25f, 0.00f}, // Default cursor
	{1001, 16, 16, 8, 8, 0.50f, 0.50f},
	{1011, 32, 32, 16, 16, 0.50f, 0.50f},
	{1000, 16, 16, 6, 1, 0.50f, 0.50f},
	{1000, 16, 16, 8, 8, 0.00f, 0.25f} // Invisible cursor
};

struct CursorData : CursorDataStruct {
	//constexpr CursorData(uint32 id, uint16 w, uint16 h, uint16 hotX, uint16 hotY, float t, float tXbox) : CursorDataStruct({id,w,h,hotX,hotY,t,tXbox}){}//: nodeID(id), width(w), height(h), hotspotX(hotX), hotspotY(hotY), transparency(t), transparencyXbox(tXbox) {};
	constexpr CursorData(int idx) : CursorDataStruct(availableCursors[idx]) {};
	Rect size() { return Rect(width, height); }
	Point getHotspot() { return Point(hotspotX, hotspotY); }
};

// class CursorData {
// public:
// 	CursorData(int idx)
// 		: _cursorData(availableCursors[idx]),
// 		  transparency(_cursorData.transparency),
// 		  nodeID(_cursorData.nodeID) {
// 	}
// 	Point getHotspot() { return Point(_cursorData.hotspotX, _cursorData.hotspotY); };
// 	Rect size() { return Rect(_cursorData.width, _cursorData.height); }
	
// 	const uint32 nodeID;
// 	const float transparency;

// private:
// 	const CursorDataStruct _cursorData;
// };

class Cursor : public Drawable {
public:
	Cursor(Myst3Engine *vm);
	virtual ~Cursor();

	void changeCursor(uint32 index);
	bool isPositionLocked() { return _lockedAtCenter; };
	void lockPosition(bool lock);

	/** Get the mouse cursor position */
	PointF getPosition() const {
		return _position;
	}
	PointF getScreenPosition() const;
	Point getOriginalGamePosition() const;

	void updatePosition(const PointF &mouse);

	void getDirection(float &pitch, float &heading);

	void draw() override;
	void setVisible(bool show);
	bool isVisible();

private:
	Myst3Engine *_vm;

	uint32 _currentCursorID;
	int32 _hideLevel;

	/** Position of the cursor */
	PointF _position;

	// Surfaces for hardware cursor (CursorMan)
	typedef Common::HashMap<uint32, Graphics::Surface *> SurfaceMap;
	SurfaceMap _surfaces;

	// Textures for manual drawing when locked at center
	typedef Common::HashMap<uint32, Texture *> TextureMap;
	TextureMap _textures;

	bool _lockedAtCenter;

	void loadAvailableCursors();
	float getTransparencyForId(uint32 cursorId);
};

} // End of namespace Myst3

#endif // CURSOR_H_
