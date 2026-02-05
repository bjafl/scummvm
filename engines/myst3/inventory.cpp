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

#include "engines/myst3/inventory.h"
#include "engines/myst3/cursor.h"
#include "engines/myst3/database.h"
#include "engines/myst3/scene.h"
#include "engines/myst3/state.h"

namespace Myst3 {

const Inventory::ItemData Inventory::_availableItems[8] = {
	{   0, 41, 47, 481 },
	{  41, 38, 50, 480 },
	{  79, 38, 49, 279 },
	{ 117, 34, 48, 277 },
	{ 151, 35, 44, 345 },
	{ 186, 35, 44, 398 },
	{ 221, 35, 44, 447 },
	{   0,  0,  0,   0 }
};

Inventory::Inventory(Myst3Engine *vm) :
		Window(),
		_vm(vm),
		_texture(nullptr) {
	initializeTexture();
}

Inventory::~Inventory() {
	delete _texture;
}

void Inventory::initializeTexture() {
	ResourceDescription desc = _vm->_resourceLoader->getRawData("GLOB", kInventoryTextureId);
	if (!desc.isValid())
		error("The inventory texture, GLOB-1204 was not found");

	TextureLoader textureLoader(*_vm->_gfx);
	_texture = textureLoader.load(desc, TextureLoader::kImageFormatTEX);
}

bool Inventory::isMouseInside() {
	PointF mouse = _vm->_cursor->getPosition();
	return getPosition().contains(mouse);
}

// Rect Inventory::getBottomBorder() const {
// 	Rect screen = _vm->_gfx->viewport();
// 	float heightScale = screen.height() / (float) Renderer::kOriginalHeight;
// 	Rect bottomBorder(screen.width(), screen.height() * heightScale);
// 	bottomBorder.translate(0, screen.height() - bottomBorder.height());
// 	return bottomBorder;
// }
void Inventory::draw() {
	RectF windowPos = getPosition();

	if (_vm->isWideScreenModEnabled()) {
		// Draw a black background to cover the main game frame
		// Use full window rect (0,0 to width,height) since we're drawing relative to viewport
		_vm->_gfx->drawRect2D(Rect(windowPos.width(), windowPos.height()), 0xFF, 0x00, 0x00, 0x00);
	}

	uint16 hoveredItemVar = hoveredItem();
	float textureScale = _texture->width / (float) kInventoryTextoreOriginalWidth;

	for (ItemList::const_iterator it = _inventory.begin(); it != _inventory.end(); it++) {
		int32 state = _vm->_state->getVar(it->var);

		// Don't draw if the item is being dragged or is hidden
		if (state == -1 || state == 0)
			continue;

		const ItemData &item = getData(it->var);

		Rect textureRect(item.textureWidth, item.textureHeight);
		textureRect.translate(item.textureX * textureScale, 0);
		
		bool itemHighlighted = it->var == hoveredItemVar || state == 2;

		if (itemHighlighted) {
			textureRect.translate(0, _texture->height / 2);
		}
		
		//TODO: Normalize?

		_vm->_gfx->drawTexturedRect2D(it->rect, textureRect, _texture);
    debugC(kDebugGraphics, "Inventory drawTexturedRect2D - screen [%.2fx%.2f], texture [%.2fx%.2f]", it->rect.width(), it->rect.height(), textureRect.width(), textureRect.height());
	}
}

void Inventory::reset() {
	_inventory.clear();
	reflow();
	updateState();
}

void Inventory::addItem(uint16 var, bool atEnd) {
	// Only add objects once to the inventory
	if (!hasItem(var)) {
		_vm->_state->setVar(var, 1);

		InventoryItem i;
		i.var = var;

		if (atEnd) {
			_inventory.push_back(i);
		} else {
			_inventory.push_front(i);
		}

		reflow();
		updateState();
	}
}

void Inventory::removeItem(uint16 var) {
	_vm->_state->setVar(var, 0);

	for (ItemList::iterator it = _inventory.begin(); it != _inventory.end(); it++) {
		if (it->var == var) {
			_inventory.erase(it);
			break;
		}
	}

	reflow();
	updateState();
}

void Inventory::addAll() {
	for (uint i = 0; _availableItems[i].var; i++)
		addItem(_availableItems[i].var, true);
}

bool Inventory::hasItem(uint16 var) {
	for (ItemList::iterator it = _inventory.begin(); it != _inventory.end(); it++) {
		if (it->var == var)
			return true;
	}

	return false;
}

const Inventory::ItemData &Inventory::getData(uint16 var) {
	for (uint i = 0; _availableItems[i].var; i++) {
		if (_availableItems[i].var == var)
			return _availableItems[i];
	}

	return _availableItems[7];
}

void Inventory::reflow() {
	uint16 itemCount = 0;
	uint16 totalWidth = 0;
	RectF windowPos = getPosition();
	// RectF originalPos = getOriginalPosition();

	// Scale factor from original to viewport
	float scale = _vm->_gfx->getScale();

	for (uint i = 0; _availableItems[i].var; i++) {
		if (hasItem(_availableItems[i].var)) {
			totalWidth += _availableItems[i].textureWidth * scale;
			itemCount++;
		}
	}

	if (itemCount >= 2)
		totalWidth += 9 * scale * (itemCount - 1);

	// Center items horizontally, position relative to window (0,0 is top-left)
	uint left = (windowPos.width() - totalWidth) / 2;

	for (ItemList::iterator it = _inventory.begin(); it != _inventory.end(); it++) {
		const ItemData &item = getData(it->var);

		PointF itemSize(item.textureWidth * scale, item.textureHeight * scale);
		uint16 top = (windowPos.height() - itemSize.y) / 2;

		// Rect is relative to the window viewport, in viewport pixels
		it->rect = Rect(Point(left, top), (int16)itemSize.x, (int16)itemSize.y);

		left += itemSize.x;

		if (itemCount >= 2)
			left += 9 * scale;
	}
}

uint16 Inventory::hoveredItem() {
	PointF mouse = _vm->_cursor->getPosition();
	mouse = screenPosToWindowPos(mouse);

	for (ItemList::const_iterator it = _inventory.begin(); it != _inventory.end(); it++) {
		if(it->rect.contains(mouse.x, mouse.y))
			return it->var;
	}

	return 0;
}

void Inventory::useItem(uint16 var) {
	switch (var) {
	case 277: // Atrus
		closeAllBooks();
		_vm->_state->setJournalAtrusState(2);
		openBook(9, kRoomJournals, 100);
		break;
	case 279: // Saavedro
		closeAllBooks();
		_vm->_state->setJournalSaavedroState(2);
		openBook(9, kRoomJournals, 200);
		break;
	case 480: // Tomahna
		closeAllBooks();
		_vm->_state->setBookStateTomahna(2);
		openBook(8, kRoomNarayan, 220);
		break;
	case 481: // Releeshahn
		closeAllBooks();
		_vm->_state->setBookStateReleeshahn(2);
		openBook(9, kRoomJournals, 300);
		break;
	case 345:
		_vm->dragSymbol(345, 1002);
		break;
	case 398:
		_vm->dragSymbol(398, 1001);
		break;
	case 447:
		_vm->dragSymbol(447, 1000);
		break;
	default:
		debug("Used inventory item %d which is not implemented", var);
	}
}

void Inventory::closeAllBooks() {
	if (_vm->_state->getJournalAtrusState())
		_vm->_state->setJournalAtrusState(1);
	if (_vm->_state->getJournalSaavedroState())
		_vm->_state->setJournalSaavedroState(1);
	if (_vm->_state->getBookStateTomahna())
		_vm->_state->setBookStateTomahna(1);
	if (_vm->_state->getBookStateReleeshahn())
		_vm->_state->setBookStateReleeshahn(1);
}

void Inventory::openBook(uint16 age, uint16 room, uint16 node) {
	if (!_vm->_state->getBookSavedNode()) {
		_vm->_state->setBookSavedAge(_vm->_state->getLocationAge());
		_vm->_state->setBookSavedRoom(_vm->_state->getLocationRoom());
		_vm->_state->setBookSavedNode(_vm->_state->getLocationNode());
	}

	_vm->_state->setLocationNextAge(age);
	_vm->_state->setLocationNextRoom(room);
	_vm->goToNode(node, kTransitionFade);
}

void Inventory::addSaavedroChapter(uint16 var) {
	_vm->_state->setVar(var, 1);
	_vm->_state->setJournalSaavedroState(2);
	_vm->_state->setJournalSaavedroChapter(var - 285);
	_vm->_state->setJournalSaavedroPageInChapter(0);
	openBook(9, kRoomJournals, 200);
}

void Inventory::loadFromState() {
	Common::Array<uint16> items = _vm->_state->getInventory();

	_inventory.clear();
	for (uint i = 0; i < items.size(); i++)
		addItem(items[i], true);
}

void Inventory::updateState() {
	Common::Array<uint16> items;
	for (ItemList::iterator it = _inventory.begin(); it != _inventory.end(); it++)
		items.push_back(it->var);

	_vm->_state->updateInventory(items);
}

RectF Inventory::getPosition() const {
	RectF screen = _vm->_gfx->viewport();

	RectF frame;
	if (_vm->isWideScreenModEnabled()) {
		frame = RectF(screen.width(), Renderer::kBottomBorderHeight);

		RectF scenePosition = _vm->_scene->getPosition();
		int16 top = CLIP<int16>(screen.height() - frame.height(), 0, scenePosition.bottom);

		frame.translate(0, top);
	} else {
		frame = RectF(screen.width(), screen.height() * Renderer::kBottomBorderHeightRelative);
		frame.translate(screen.left, screen.top + screen.height() * (Renderer::kTopBorderHeightRelative + Renderer::kFrameHeightRelative));
	}

	return frame;
}

RectF Inventory::getOriginalPosition() const {
	RectF originalPosition = RectF(Renderer::kOriginalWidth, Renderer::kBottomBorderHeight);
	originalPosition.translate(0, Renderer::kTopBorderHeight + Renderer::kFrameHeight);
	return originalPosition;
}

void Inventory::updateCursor() {
	uint16 item = hoveredItem();
	if (item > 0) {
		_vm->_cursor->changeCursor(1);
	} else {
		_vm->_cursor->changeCursor(8);
	}
}

DragItem::DragItem(Myst3Engine *vm, uint id):
		_vm(vm),
		_texture(nullptr),
		_frame(1) {
	// Draw on the whole screen
	_isConstrainedToWindow = false;
	
	ResourceDescription movieDesc = _vm->_resourceLoader->getStillMovie("DRAG", id);

	if (!movieDesc.isValid())
		error("Movie %d does not exist", id);

	// Load the video
	VideoLoader videoLoader;
	_movieStream = videoLoader.load(movieDesc);
	assert(_movieStream);
	_bink.setOutputPixelFormat(Texture::getRGBAPixelFormat());
	if (!_bink.loadStream(_movieStream)) {
		error("Invalid Bink video file '%s-%d'", "DRAG", id);
	}
	_bink.start();

	const Graphics::Surface *frame = _bink.decodeNextFrame();
	_texture = _vm->_gfx->createTexture2D(frame);

	if (movieDesc.getType() == Archive::kModdedMovie) {
		// For modded resources, the screen size is that from the original file
		 ResourceDescription::VideoData videoData = movieDesc.getVideoData();
		_screenSize = Rect(videoData.width, videoData.height);
	} else {
		_screenSize = _texture->size();
	}
}

DragItem::~DragItem() {
	delete _texture;
}

void DragItem::drawOverlay() {
	RectF itemRect = getPosition();

	// _vm->_gfx->setViewport(viewport, false);
	_vm->_gfx->drawTexturedRect2D(itemRect, Rect(_texture->width, _texture->height), _texture, 0.99f);
}

void DragItem::setFrame(uint16 frame) {
	if (frame != _frame) {
		_frame = frame;
		_bink.seekToFrame(frame - 1);
		const Graphics::Surface *s = _bink.decodeNextFrame();
		_texture->update(s);
	}
}

RectF DragItem::getPosition() {
	PointF mouse = _vm->_cursor->getPosition();
	RectF viewport = _vm->_gfx->viewport();
	float scale = _vm->_gfx->getScale();

	Rect itemSize = Rect(_screenSize.width() * scale, _screenSize.height() * scale);
	        

	Point itemTargetCenter(
	            CLIP<float>(mouse.x, viewport.left + itemSize.width()  / 2, viewport.right  - itemSize.width()  / 2),
	            CLIP<float>(mouse.y, viewport.top  + itemSize.height() / 2, viewport.bottom - itemSize.height() / 2)
	);
	Point topLeftTarget = itemTargetCenter - itemSize.center();
	return Rect(topLeftTarget, itemSize.width(), itemSize.height());
}

} // End of namespace Myst3
