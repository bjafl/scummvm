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
#include "engines/myst3/myst3.h"
#include "engines/myst3/nodeframe.h"
#include "engines/myst3/scene.h"
#include "engines/myst3/state.h"

namespace Myst3 {

NodeFrame::NodeFrame(Myst3Engine *vm, Common::String &room, uint16 id) :
		Node(vm, id) {
	// Common::String roomName = _vm->getCurrentRoomName();
	ResourceDescription bitmap = _vm->_resourceLoader->getFrameBitmap(room, id);
	_faces[0] = new Face(_vm);
	_faces[0]->setTextureFromBitmap(&bitmap);
}

NodeFrame::~NodeFrame() {
}

void NodeFrame::draw() {
	Rect screenRect;

	// Size and position of the frame
	if (_vm->_state->getViewType() == kMenu) {
		screenRect = _vm->_gfx->viewport();
		//screenRect = _vm->_gfx->origAspectRatioViewport();
	} else {
		screenRect = _vm->_gfx->frameViewport();
	}

	// Update the OpenGL texture if needed
	_faces[0]->uploadTexture();

	// Used fragment of texture (full texture)
	Rect textureRect = Rect(_faces[0]->_texture->width, _faces[0]->_texture->height);

	// Draw
	_vm->_gfx->drawTexturedRect2D(screenRect, textureRect, _faces[0]->_texture);
    debugC(kDebugGraphics, "NodeFrame drawTexturedRect2D - screen [%dx%d], texture [%dx%d]", screenRect.width(), screenRect.height(), textureRect.width(), textureRect.height());
}

} // End of namespace Myst3
