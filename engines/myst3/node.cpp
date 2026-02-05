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
#include "engines/myst3/database.h"
#include "engines/myst3/effects.h"
#include "engines/myst3/node.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/state.h"
#include "engines/myst3/subtitles.h"

#include "common/config-manager.h"
#include "common/debug.h"
#include "engines/myst3/rect.h"

namespace Myst3 {

void Face::setTextureFromBitmap(const ResourceDescription *bitmap) {
	TextureLoader loader(*_vm->_gfx);
	if (_bitmap) {
		_bitmap->free();
		delete _bitmap;
	}
	_bitmap = loader.loadSurface(*bitmap, TextureLoader::kImageFormatJPEG);
	if (_is3D) {
		_texture = _vm->_gfx->createTexture3D(_bitmap);
	} else {
		_texture = _vm->_gfx->createTexture2D(_bitmap);
	}

	// Set the whole texture as dirty
	addTextureDirtyRect(RectF(_bitmap->w, _bitmap->h));
}

Face::Face(Myst3Engine *vm, bool is3D) :
		_vm(vm),
		_is3D(is3D),
		_textureDirty(true),
		_texture(nullptr),
		_bitmap(nullptr),
		_finalBitmap(nullptr) {
}

void Face::addTextureDirtyRect(const RectF &rect) {
	if (!_textureDirty) {
		_textureDirtyRect = rect;
	} else {
		_textureDirtyRect.extend(rect);
	}

	_textureDirty = true;
}

void Face::uploadTexture() {
	if (_textureDirty) {
		if (_finalBitmap)
			_texture->updatePartial(_finalBitmap, _textureDirtyRect);
		else
			_texture->updatePartial(_bitmap, _textureDirtyRect);

		_textureDirty = false;
	}
}

Face::~Face() {
	if (_bitmap) {
		_bitmap->free();
		delete _bitmap;
		_bitmap = nullptr;
	}

	if (_finalBitmap) {
		_finalBitmap->free();
		delete _finalBitmap;
	}

	if (_texture) {
		delete _texture;
	}
}

Node::Node(Myst3Engine *vm, uint16 id) :
		_vm(vm),
		_id(id),
		_subtitles(nullptr) {
	for (uint i = 0; i < ARRAYSIZE(_faces); i++)
		_faces[i] = nullptr;
}

void Node::initEffects() {
	resetEffects();

	debugC(kDebugNode, "Node::initEffects: node=%d, viewType=%d", _id, _vm->_state->getViewType());

	// Log face dimensions for debugging modded textures
	for (uint i = 0; i < 6; i++) {
		if (_faces[i] && _faces[i]->_bitmap) {
			debugC(kDebugNode, "  Face %d: bitmap=%dx%d", i, _faces[i]->_bitmap->w, _faces[i]->_bitmap->h);
		}
	}

	if (_vm->_state->getViewType() == kMenu) {
		// The node init script does not clear the magnet effect state.
		// Here we ignore effects on menu nodes so we don't try to
		// to load the magnet effect when opening the main menu on Amateria.
		debugC(kDebugNode, "  Skipping effects for menu node");
		return;
	}

	Common::String room = _vm->getCurrentRoomName();
	if (_vm->_state->getWaterEffects()) {
		Effect *effect = WaterEffect::create(_vm, room, _id);
		if (effect) {
			_effects.push_back(effect);
			_vm->_state->setWaterEffectActive(true);
			debugC(kDebugNode, "  Created WaterEffect");
		}
	}

	Effect *effect = MagnetEffect::create(_vm, room, _id);
	if (effect) {
		_effects.push_back(effect);
		_vm->_state->setMagnetEffectActive(true);
		debugC(kDebugNode, "  Created MagnetEffect");
	}

	effect = LavaEffect::create(_vm, room, _id);
	if (effect) {
		_effects.push_back(effect);
		_vm->_state->setLavaEffectActive(true);
		debugC(kDebugNode, "  Created LavaEffect");
	}

	effect = ShieldEffect::create(_vm, _id);
	if (effect) {
		_effects.push_back(effect);
		_vm->_state->setShieldEffectActive(true);
		debugC(kDebugNode, "  Created ShieldEffect");
	}

	debugC(kDebugNode, "  Total effects: %d", _effects.size());
}

void Node::resetEffects() {
	for (uint i = 0; i < _effects.size(); i++) {
		delete _effects[i];
	}
	_effects.clear();
}

Node::~Node() {
	for (uint i = 0; i < _spotItems.size(); i++) {
		delete _spotItems[i];
	}
	_spotItems.clear();

	resetEffects();

	_vm->_state->setWaterEffectActive(false);
	_vm->_state->setMagnetEffectActive(false);
	_vm->_state->setLavaEffectActive(false);
	_vm->_state->setShieldEffectActive(false);

	for (int i = 0; i < 6; i++) {
		delete _faces[i];
	}

	delete _subtitles;
}

void Node::loadSpotItem(const Common::String &room, uint16 id, int16 condition, bool fade) {
	SpotItem *spotItem = new SpotItem(_vm);

	spotItem->setCondition(condition);
	spotItem->setFade(fade);
	spotItem->setFadeVar(abs(condition));

	debugC(kDebugNode, "Node::loadSpotItem: room=%s, id=%d, condition=%d, fade=%d",
	       room.c_str(), id, condition, fade);

	ResourceDescriptionArray resources = _vm->_resourceLoader->listSpotItemImages(room, id);
	TextureLoader textureLoader(*_vm->_gfx);

	// Determine original face dimensions based on view type for position scaling
	// SpotItem metadata positions (u, v) are in original game coordinates,
	// but face bitmaps may be upscaled (modded textures), so we need to scale
	float origFaceW, origFaceH;
	ViewType viewType = _vm->_state->getViewType();
	if (viewType == kCube) {
		origFaceW = 640.0f;
		origFaceH = 640.0f;
	} else if (viewType == kFrame) {
		origFaceW = Renderer::kOriginalWidth;
		origFaceH = Renderer::kFrameHeight;
	} else {
		origFaceW = Renderer::kOriginalWidth;
		origFaceH = Renderer::kOriginalHeight;
	}

	for (uint i = 0; i < resources.size(); i++) {
		const ResourceDescription &image = resources[i];
		ResourceDescription::SpotItemData spotItemData = image.getSpotItemData();

		debugC(kDebugNode, "  SpotItem face %d: type=%d, u=%d, v=%d",
		       i, image.getType(), spotItemData.u, spotItemData.v);

		assert(i < 6 && "Node::loadSpotItem: face index out of bounds");
		assert(_faces[i] && "Node::loadSpotItem: face is null");
		assert(_faces[i]->_bitmap && "Node::loadSpotItem: face bitmap is null");

		// Scale positions from original face coordinates to actual face bitmap coordinates
		float scaleX = _faces[i]->_bitmap->w / origFaceW;
		float scaleY = _faces[i]->_bitmap->h / origFaceH;
		float scaledU = spotItemData.u * scaleX;
		float scaledV = spotItemData.v * scaleY;

		debugC(kDebugNode, "    Scale: %.2fx%.2f, scaled pos (%.1f, %.1f)",
		       scaleX, scaleY, scaledU, scaledV);

		SpotItemFace *spotItemFace = new SpotItemFace(_faces[i], scaledU, scaledV);

		Graphics::Surface *bitmapSurface = textureLoader.loadSurface(image, TextureLoader::kImageFormatJPEG);
		assert(bitmapSurface && "Node::loadSpotItem: failed to load bitmap surface");
		spotItemFace->loadData(bitmapSurface);

		debugC(kDebugNode, "    Loaded bitmap: %dx%d, face bitmap: %dx%d",
		       bitmapSurface->w, bitmapSurface->h,
		       _faces[i]->_bitmap->w, _faces[i]->_bitmap->h);

		// Verify spot item fits within face (using scaled positions)
		assert(scaledU + bitmapSurface->w <= _faces[i]->_bitmap->w &&
		       "Node::loadSpotItem: spot item exceeds face width");
		assert(scaledV + bitmapSurface->h <= _faces[i]->_bitmap->h &&
		       "Node::loadSpotItem: spot item exceeds face height");

		bitmapSurface->free();
		delete bitmapSurface;

		// SpotItems with an always true conditions cannot be undrawn.
		// Draw them now to make sure the "non drawn backups" for other, potentially
		// overlapping SpotItems have them drawn.
		if (condition == 1) {
			spotItemFace->draw();
		}

		spotItem->addFace(spotItemFace);
	}
	_spotItems.push_back(spotItem);
}

SpotItemFace *Node::loadMenuSpotItem(int16 condition, const RectF &rect) {
	SpotItem *spotItem = new SpotItem(_vm);

	spotItem->setCondition(condition);
	spotItem->setFade(false);
	spotItem->setFadeVar(abs(condition));

	assert(_faces[0] && "Node::loadMenuSpotItem: face 0 is null");
	assert(_faces[0]->_bitmap && "Node::loadMenuSpotItem: face 0 bitmap is null");

	// rect is in original game coordinates
	// Scale to face bitmap coordinates (face may be upscaled for modded textures)
	float origW = (_vm->_state->getViewType() == kMenu)
	              ? Renderer::kOriginalWidth : Renderer::kOriginalWidth;
	float origH = (_vm->_state->getViewType() == kMenu)
	              ? Renderer::kOriginalHeight : Renderer::kFrameHeight;
	float scaleX = _faces[0]->_bitmap->w / origW;
	float scaleY = _faces[0]->_bitmap->h / origH;
	RectF scaledRect(rect.left * scaleX, rect.top * scaleY,
	                 rect.right * scaleX, rect.bottom * scaleY);

	debugC(kDebugNode, "Node::loadMenuSpotItem: condition=%d, origRect=[%d,%d,%d,%d], scaledRect=[%d,%d,%d,%d], face0 bitmap=%dx%d, scale=%.2fx%.2f",
	       condition, rect.left, rect.top, rect.right, rect.bottom,
	       scaledRect.left, scaledRect.top, scaledRect.right, scaledRect.bottom,
	       _faces[0]->_bitmap->w, _faces[0]->_bitmap->h, scaleX, scaleY);

	assert(scaledRect.left >= 0 && scaledRect.top >= 0 && "Node::loadMenuSpotItem: rect has negative coordinates");
	assert(scaledRect.width() > 0 && scaledRect.height() > 0 && "Node::loadMenuSpotItem: rect has zero dimensions");
	assert(scaledRect.right <= _faces[0]->_bitmap->w && scaledRect.bottom <= _faces[0]->_bitmap->h &&
	       "Node::loadMenuSpotItem: rect exceeds face dimensions");

	SpotItemFace *spotItemFace = new SpotItemFace(_faces[0], scaledRect.left, scaledRect.top);
	spotItemFace->initBlack(scaledRect.width(), scaledRect.height());

	spotItem->addFace(spotItemFace);

	_spotItems.push_back(spotItem);

	return spotItemFace;
}

void Node::loadSubtitles(const Common::String &room, uint32 id) {
	_subtitles = Subtitles::create(_vm, room, id);
}

bool Node::hasSubtitlesToDraw() {
	if (!_subtitles || _vm->_state->getSpotSubtitle() <= 0)
		return false;

	if (!_vm->isTextLanguageEnglish() && _vm->_state->getLocationRoom() == kRoomNarayan) {
		// The words written on the walls in Narayan are always in English.
		// Show the subtitles regardless of the "subtitles" setting if the game language is not English.
		return true;
	}

	return ConfMan.getBool("subtitles");
}

void Node::drawOverlay() {
	if (hasSubtitlesToDraw()) {
		uint subId = _vm->_state->getSpotSubtitle();
		_subtitles->setFrame(15 * subId + 1);
		_vm->_gfx->renderWindowOverlay(_subtitles);
	}
}

void Node::update() {
	// First undraw ...
	for (uint i = 0; i < _spotItems.size(); i++) {
		_spotItems[i]->updateUndraw();
	}

	// ... then redraw
	for (uint i = 0; i < _spotItems.size(); i++) {
		_spotItems[i]->updateDraw();
	}

	// Skip CPU effect processing if using GPU shader effects
	// The effects will be applied in the draw() call via the shader
	if (_vm->_gfx->supportsShaderEffects() && !_effects.empty()) {
		return;
	}

	bool needsUpdate = false;
	for (uint i = 0; i < _effects.size(); i++) {
		needsUpdate |= _effects[i]->update();
	}

	// Apply the effects for all the faces (CPU fallback path)
	for (uint faceId = 0; faceId < 6; faceId++) {
		Face *face = _faces[faceId];

		if (face == nullptr)
			continue; // No such face in this node

		if (!isFaceVisible(faceId)) {
			continue; // This face is not currently visible
		}

		uint effectsForFace = 0;
		for (uint i = 0; i < _effects.size(); i++) {
			if (_effects[i]->hasFace(faceId))
				effectsForFace++;
		}

		if (effectsForFace == 0)
			continue;
		if (!needsUpdate && !face->isTextureDirty())
			continue;

		// Alloc the target surface if necessary
		if (!face->_finalBitmap) {
			face->_finalBitmap = new Graphics::Surface();
		}
		face->_finalBitmap->copyFrom(*face->_bitmap);

		if (effectsForFace == 1) {
			_effects[0]->applyForFace(faceId, face->_bitmap, face->_finalBitmap);

			face->addTextureDirtyRect(_effects[0]->getUpdateRectForFace(faceId));
		} else if (effectsForFace == 2) {
			// TODO: Keep the same temp surface to avoid heap fragmentation ?
			Graphics::Surface *tmp = new Graphics::Surface();
			tmp->copyFrom(*face->_bitmap);

			_effects[0]->applyForFace(faceId, face->_bitmap, tmp);
			_effects[1]->applyForFace(faceId, tmp, face->_finalBitmap);

			tmp->free();
			delete tmp;

			face->addTextureDirtyRect(_effects[0]->getUpdateRectForFace(faceId));
			face->addTextureDirtyRect(_effects[1]->getUpdateRectForFace(faceId));
		} else {
			error("Unable to render more than 2 effects per faceId (%d)", effectsForFace);
		}
	}
}

SpotItem::SpotItem(Myst3Engine *vm) :
	_vm(vm) {
}

SpotItem::~SpotItem() {
	for (uint i = 0; i < _faces.size(); i++) {
		delete _faces[i];
	}
}

void SpotItem::updateUndraw() {
	for (uint i = 0; i < _faces.size(); i++) {
		if (!_vm->_state->evaluate(_condition) && _faces[i]->isDrawn()) {
			_faces[i]->undraw();
		}
	}
}

void SpotItem::updateDraw() {
	for (uint i = 0; i < _faces.size(); i++) {
		if (_enableFade) {
			uint16 newFadeValue = _vm->_state->getVar(_fadeVar);

			if (_faces[i]->getFadeValue() != newFadeValue) {
				_faces[i]->setFadeValue(newFadeValue);
				_faces[i]->setDrawn(false);
			}
		}

		if (_vm->_state->evaluate(_condition) && !_faces[i]->isDrawn()) {
			if (_enableFade)
				_faces[i]->fadeDraw();
			else
				_faces[i]->draw();
		}
	}
}

SpotItemFace::SpotItemFace(Face *face, float posX, float posY):
		_face(face),
		_posX(posX),
		_posY(posY),
		_drawn(false),
		_bitmap(nullptr),
		_notDrawnBitmap(nullptr),
		_fadeValue(0) {
}

SpotItemFace::~SpotItemFace() {
	if (_bitmap) {
		_bitmap->free();
		delete _bitmap;
		_bitmap = nullptr;
	}

	if (_notDrawnBitmap) {
		_notDrawnBitmap->free();
		delete _notDrawnBitmap;
		_notDrawnBitmap = nullptr;
	}
}

void SpotItemFace::initBlack(float width, float height) {
	if (_bitmap) {
		_bitmap->free();
	}

	_bitmap = new Graphics::Surface();
	_bitmap->create(width, height, Texture::getRGBAPixelFormat());

	initNotDrawn(width, height);

	_drawn = false;
}

void SpotItemFace::loadData(const Graphics::Surface *bitmap) {
	assert(bitmap->format == Texture::getRGBAPixelFormat());
	// Convert active SpotItem image to raw data
	if (_bitmap) {
		_bitmap->free();
		delete _bitmap;
	}
	_bitmap = new Graphics::Surface();
	_bitmap->copyFrom(*bitmap);

	initNotDrawn(_bitmap->w, _bitmap->h);
}

void SpotItemFace::updateData(const Graphics::Surface *surface) {
	assert(_bitmap && surface);
	assert(surface->format == Texture::getRGBAPixelFormat());

	//_bitmap->free();
	_bitmap->copyFrom(*surface);

	_drawn = false;
}

void SpotItemFace::clear() {
	memset(_bitmap->getPixels(), 0, _bitmap->pitch * _bitmap->h);

	_drawn = false;
}

void SpotItemFace::initNotDrawn(float width, float height) {
	// Copy not drawn SpotItem image from face
	_notDrawnBitmap = new Graphics::Surface();
	_notDrawnBitmap->create(width, height, Texture::getRGBAPixelFormat());

	for (uint i = 0; i < height; i++) {
		memcpy(_notDrawnBitmap->getBasePtr(0, i), _face->_bitmap->getBasePtr(_posX, _posY + i), width * 4);
	}
}

RectF SpotItemFace::getFaceRect() const {
	assert(_bitmap);

	RectF r = RectF(_bitmap->w, _bitmap->h);

	// Scale from original game coordinates
	
	r.translate(_posX, _posY);
	return r;
}

void SpotItemFace::draw() {
	for (int i = 0; i < _bitmap->h; i++) {
		memcpy(_face->_bitmap->getBasePtr(_posX, _posY + i), _bitmap->getBasePtr(0, i), _bitmap->w * 4);
	}

	_drawn = true;
	_face->addTextureDirtyRect(getFaceRect());
}

void SpotItemFace::undraw() {
	for (int i = 0; i < _notDrawnBitmap->h; i++) {
		memcpy(_face->_bitmap->getBasePtr(_posX, _posY + i), _notDrawnBitmap->getBasePtr(0, i), _notDrawnBitmap->w * 4);
	}

	_drawn = false;
	_face->addTextureDirtyRect(getFaceRect());
}

void SpotItemFace::fadeDraw() {
	uint16 fadeValue = CLIP<uint16>(_fadeValue, 0, 100);

	for (int i = 0; i < _bitmap->h; i++) {
		byte *ptrND = (byte *)_notDrawnBitmap->getBasePtr(0, i);
		byte *ptrD = (byte *)_bitmap->getBasePtr(0, i);
		byte *ptrDest = (byte *)_face->_bitmap->getBasePtr(_posX, _posY + i);

		for (int j = 0; j < _bitmap->w; j++) {
			byte rND = *ptrND++;
			byte gND = *ptrND++;
			byte bND = *ptrND++;
			ptrND++; // Alpha
			byte rD = *ptrD++;
			byte gD = *ptrD++;
			byte bD = *ptrD++;
			ptrD++; // Alpha

			// TODO: optimize ?
			*ptrDest++ = rND * (100 - fadeValue) / 100 + rD * fadeValue / 100;
			*ptrDest++ = gND * (100 - fadeValue) / 100 + gD * fadeValue / 100;
			*ptrDest++ = bND * (100 - fadeValue) / 100 + bD * fadeValue / 100;
			ptrDest++; // Alpha
		}
	}

	_drawn = true;
	_face->addTextureDirtyRect(getFaceRect());
}

} // end of namespace Myst3
