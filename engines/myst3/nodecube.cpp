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
#include "engines/myst3/effects.h"
#include "engines/myst3/nodecube.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/state.h"

#include "common/debug.h"

namespace Myst3 {

NodeCube::NodeCube(Myst3Engine *vm, Common::String &room, uint16 id) :
		Node(vm, id),
		_shieldPatternTexture(nullptr),
		_effectTexturesInitialized(false) {
	_is3D = true;

	for (int i = 0; i < 6; i++) {
		_effectMaskTextures[i] = nullptr;
		_effectMaskTextures2[i] = nullptr;
	}

	// Common::String roomName = _vm->getCurrentRoomName();
	for (int i = 0; i < 6; i++) {
		ResourceDescription bitmap = _vm->_resourceLoader->getCubeBitmap(room, id, i);
		if (!bitmap.isValid())
			error("Face %d does not exist", id);

		_faces[i] = new Face(_vm, true);
		_faces[i]->setTextureFromBitmap(&bitmap);
	}
}

NodeCube::~NodeCube() {
	freeEffectMaskTextures();
}

static Graphics::Surface *convertMaskToRGBA(const Graphics::Surface *mask) {
	// Effect masks are 8-bit grayscale (CLUT8), need to convert to RGBA for GPU texture
	Graphics::Surface *rgba = new Graphics::Surface();
	rgba->create(mask->w, mask->h, Texture::getRGBAPixelFormat());

	const uint8 *src = (const uint8 *)mask->getPixels();
	uint8 *dst = (uint8 *)rgba->getPixels();

	for (int i = 0; i < mask->w * mask->h; i++) {
		uint8 value = src[i];
		// Store grayscale value in R channel, shader reads .r component
		dst[i * 4 + 0] = value;  // R
		dst[i * 4 + 1] = value;  // G
		dst[i * 4 + 2] = value;  // B
		dst[i * 4 + 3] = 255;    // A
	}

	return rgba;
}

void NodeCube::initEffectMaskTextures() {
	if (_effectTexturesInitialized || !_vm->_gfx->supportsShaderEffects()) {
		return;
	}

	_effectTexturesInitialized = true;

	// Create textures from effect masks
	for (uint i = 0; i < _effects.size(); i++) {
		Effect *effect = _effects[i];
		const Effect::FaceMaskArray &masks = effect->facesMasks();

		// Water and Lava use texEffect1 (slot 1)
		// Magnet and Shield use texEffect2 (slot 2)
		bool isPrimaryEffect = (effect->type() == kEffectWater || effect->type() == kEffectLava);
		Texture **targetTextures = isPrimaryEffect ? _effectMaskTextures : _effectMaskTextures2;

		for (uint faceId = 0; faceId < masks.size() && faceId < 6; faceId++) {
			if (masks[faceId] && masks[faceId]->surface && !targetTextures[faceId]) {
				// Convert 8-bit grayscale mask to RGBA for GPU texture
				Graphics::Surface *rgbaMask = convertMaskToRGBA(masks[faceId]->surface);
				targetTextures[faceId] = _vm->_gfx->createTexture3D(rgbaMask);
				rgbaMask->free();
				delete rgbaMask;
			}
		}

		// Create shield pattern texture if this is a shield effect
		if (effect->type() == kEffectShield && !_shieldPatternTexture) {
			ShieldEffect *shieldEffect = static_cast<ShieldEffect *>(effect);
			// Shield pattern is also 8-bit grayscale
			Graphics::Surface *rgbaPattern = convertMaskToRGBA(&shieldEffect->pattern());
			_shieldPatternTexture = _vm->_gfx->createTexture3D(rgbaPattern);
			rgbaPattern->free();
			delete rgbaPattern;
		}
	}
}

void NodeCube::freeEffectMaskTextures() {
	for (int i = 0; i < 6; i++) {
		delete _effectMaskTextures[i];
		_effectMaskTextures[i] = nullptr;
		delete _effectMaskTextures2[i];
		_effectMaskTextures2[i] = nullptr;
	}
	delete _shieldPatternTexture;
	_shieldPatternTexture = nullptr;
	_effectTexturesInitialized = false;
}

void NodeCube::draw() {
	// Update the OpenGL textures if needed
	for (uint i = 0; i < 6; i++) {
		if (_faces[i]->isTextureDirty() && isFaceVisible(i)) {
			_faces[i]->uploadTexture();
		}
	}

	Texture *textures[6];
	for (uint i = 0; i < 6; i++) {
		textures[i] = _faces[i]->_texture;
	}

	// Use GPU-accelerated effects if supported and effects are active
	if (_vm->_gfx->supportsShaderEffects() && !_effects.empty()) {
		// Initialize effect mask textures if needed
		initEffectMaskTextures();

		_vm->_gfx->drawCubeWithEffects(textures, _effectMaskTextures, _shieldPatternTexture,
		                               _effects, _vm->_state);
	} else {
		_vm->_gfx->drawCube(textures);
	}
}

bool NodeCube::isFaceVisible(uint faceId) {
	return _vm->_gfx->isCubeFaceVisible(faceId);
}

} // End of namespace Myst3
