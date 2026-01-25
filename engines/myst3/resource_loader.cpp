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

#include "engines/myst3/resource_loader.h"

#include "engines/myst3/archive.h"
#include "engines/myst3/dds.h"
#include "engines/myst3/dds_decompress.h"
#include "engines/myst3/gfx.h"
#include "engines/myst3/myst3.h"

#include "common/archive.h"
#include "common/config-manager.h"
#include "common/fs.h"

#include "image/bmp.h"
#include "image/jpeg.h"
#include "image/png.h"

namespace Myst3 {

ResourceLoader::~ResourceLoader() {
	unloadRoomArchives();

	for (uint i = 0; i < _commonArchives.size(); i++) {
		delete _commonArchives[i];
	}
}

void ResourceLoader::addMod(const Common::String &name) {
	_mods.push_back(name);
	debugC(kDebugModding, "Registered mod '%s'", name.c_str());
}

void ResourceLoader::addArchive(const Common::String &filename, bool mandatory) {
	for (uint i = 0; i < _mods.size(); i++) {
		Common::String modFilename = Common::String::format("mods/%s/%s.patch", _mods[i].c_str(), filename.c_str());
		Archive *modArchive = new Archive();
		if (modArchive->open(modFilename.c_str(), "")) {
			_commonArchives.push_back(modArchive);
			debugC(kDebugModding, "Loaded mod archive '%s'", modFilename.c_str());
		} else {
			delete modArchive;
		}
	}

	Archive *archive = new Archive();
	if (archive->open(filename.c_str(), "")) {
		_commonArchives.push_back(archive);
		return;
	}
	// else:
	delete archive;
	if (mandatory) {
		error("Unable to open archive %s", filename.c_str());
	}
}

void ResourceLoader::unloadRoomArchives() {
	for (uint i = 0; i < _roomArchives.size(); i++) {
		delete _roomArchives[i];
	}
	_roomArchives.clear();
	_currentRoom.clear();
}

void ResourceLoader::loadRoomArchives(const Common::String &room) {
	unloadRoomArchives();

	// Check 'mods' subdirs for matching archive patches and load results
	for (uint i = 0; i < _mods.size(); i++) {
		Common::String modNodeFile = Common::String::format("mods/%s/%snodes.m3a.patch", _mods[i].c_str(), room.c_str());
		Archive *modNodeArchive = new Archive();
		if (modNodeArchive->open(modNodeFile.c_str(), room.c_str())) {
			_roomArchives.push_back(modNodeArchive);
			debugC(kDebugModding, "Loaded mod archive '%s'", modNodeFile.c_str());
		} else {
			delete modNodeArchive;
		}
	}

	// Load original room archive
	Common::String roomFile = Common::String::format("%snodes.m3a", room.c_str());
	Archive *roomArchive = new Archive();
	if (!roomArchive->open(roomFile.c_str(), room.c_str())) {
		delete roomArchive;
		error("Unable to open archive %s", roomFile.c_str());
	}

	_roomArchives.push_back(roomArchive);
	_currentRoom = room;
}

ResourceDescription ResourceLoader::getFileDescription(const Common::String &room, uint32 index, uint16 face,
													   Archive::ResourceType type) const {
	debugC(kDebugNode, "Getting file description for: room=%s, index=%d, face=%d, type=%d", room.c_str(), index, face, type);
	if (room.empty()) {
		error("No archive room name found when looking up resource %d-%d.%d", index, face, type);
	}

	// Search common archives
	for (uint archiveIndex = 0; archiveIndex < _commonArchives.size(); archiveIndex++) {
		ResourceDescription desc = _commonArchives[archiveIndex]->getDescription(room, index, face, type);
		if (desc.isValid()) {
			return desc;
		}
	}

	// Search currently loaded node archives
	for (uint archiveIndex = 0; archiveIndex < _roomArchives.size(); archiveIndex++) {
		ResourceDescription desc = _roomArchives[archiveIndex]->getDescription(room, index, face, type);
		if (desc.isValid()) {
			return desc;
		}
	}

	return ResourceDescription();
}

ResourceDescriptionArray ResourceLoader::listFilesMatching(const Common::String &room, uint32 index,
														   Archive::ResourceType type) const {
	debugC(kDebugNode, "Listing files matching: room=%s, index=%d, type=%d", room.c_str(), index, type);
	if (room.empty()) {
		error("No archive room name found when looking up resource %d.%d", index, type);
	}

	for (uint archiveIndex = 0; archiveIndex < _commonArchives.size(); archiveIndex++) {
		ResourceDescriptionArray list = _commonArchives[archiveIndex]->listFilesMatching(room, index, type);
		if (!list.empty()) {
			return list;
		}
	}

	for (uint archiveIndex = 0; archiveIndex < _roomArchives.size(); archiveIndex++) {
		ResourceDescriptionArray list = _roomArchives[archiveIndex]->listFilesMatching(room, index, type);
		if (!list.empty()) {
			return list;
		}
	}

	return ResourceDescriptionArray();
}

ResourceDescription ResourceLoader::getFrameBitmap(const Common::String &room, uint16 nodeId) const {
	debugC(kDebugNode, "Getting frame bitmap for: room=%s, nodeId=%d, faceId=%d", room.c_str(), nodeId);
	ResourceDescription resource = getFileDescription(room, nodeId, 0, Archive::kModdedFrame);

	if (!resource.isValid()) {
		resource = getFileDescription(room, nodeId, 1, Archive::kModdedFrame);
	}

	if (!resource.isValid()) {
		resource = getFileDescription(room, nodeId, 1, Archive::kLocalizedFrame);
	}

	if (!resource.isValid()) {
		resource = getFileDescription(room, nodeId, 0, Archive::kFrame);
	}

	if (!resource.isValid()) {
		resource = getFileDescription(room, nodeId, 1, Archive::kFrame);
	}

	if (!resource.isValid()) {
		error("Frame %d does not exist in room %s", nodeId, room.c_str());
	}

	return resource;
}

ResourceDescription ResourceLoader::getCubeBitmap(const Common::String &room, uint16 nodeId, uint16 faceId) const {
	debugC(kDebugNode, "Getting cube bitmap for: room=%s, nodeId=%d, faceId=%d", room.c_str(), nodeId, faceId);
	ResourceDescription resource = getFileDescription(room, nodeId, faceId + 1, Archive::kModdedCubeFace);

	if (!resource.isValid()) {
		resource = getFileDescription(room, nodeId, faceId + 1, Archive::kCubeFace);
	}

	if (!resource.isValid())
		error("Unable to load face %d from node %d does not exist", faceId, nodeId);

	return resource;
}

ResourceDescription ResourceLoader::getMovie(const Common::String &room, uint16 movieId) const {
	debugC(kDebugNode, "Getting movie for: room=%s, movieId=%d", room.c_str(), movieId);
	ResourceDescription resource = getFileDescription(room, movieId, 0, Archive::kModdedMovie);

	if (!resource.isValid())
		resource = getFileDescription(room, movieId, 0, Archive::kMultitrackMovie);

	if (!resource.isValid())
		resource = getFileDescription(room, movieId, 0, Archive::kDialogMovie);

	if (!resource.isValid())
		resource = getFileDescription(room, movieId, 0, Archive::kStillMovie);

	if (!resource.isValid())
		resource = getFileDescription(room, movieId, 0, Archive::kMovie);

	return resource;
}

ResourceDescription ResourceLoader::getStillMovie(const Common::String &room, uint16 movieId) const {
	debugC(kDebugNode, "Getting still movie for: room=%s, movieId=%d", room.c_str(), movieId);
	ResourceDescription resource = getFileDescription(room, movieId, 0, Archive::kModdedMovie);

	if (!resource.isValid())
		resource = getFileDescription(room, movieId, 0, Archive::kStillMovie);

	return resource;
}

ResourceDescription ResourceLoader::getDialogMovie(const Common::String &room, uint16 movieId) const {
	debugC(kDebugNode, "Getting dialog movie for: room=%s, movieId=%d", room.c_str(), movieId);
	ResourceDescription resource = getFileDescription(room, movieId, 0, Archive::kModdedMovie);

	if (!resource.isValid())
		resource = getFileDescription(room, movieId, 0, Archive::kDialogMovie);

	if (!resource.isValid())
		resource = getFileDescription(room, movieId, 0, Archive::kStillMovie);

	return resource;
}

ResourceDescription ResourceLoader::getRawData(const Common::String &room, uint16 id) const {
	debugC(kDebugNode, "Getting raw data for: room=%s, id=%d", room.c_str(), id);
	ResourceDescription resource = getFileDescription(room, id, 0, Archive::kModdedRawData);

	if (!resource.isValid())
		resource = getFileDescription(room, id, 0, Archive::kRawData);

	return resource;
}

ResourceDescriptionArray ResourceLoader::listSpotItemImages(const Common::String &room, uint16 spotItemId) const {
	debugC(kDebugNode, "Listing spot item images for: room=%s, spotItemId=%d", room.c_str(), spotItemId);
	ResourceDescriptionArray resources;
	resources.push_back(listFilesMatching(room, spotItemId, Archive::kModdedSpotItem));

	if (resources.empty()) {
		resources.push_back(listFilesMatching(room, spotItemId, Archive::kLocalizedSpotItem));
		resources.push_back(listFilesMatching(room, spotItemId, Archive::kSpotItem));
	}

	return resources;
}

bool ResourceLoader::checkForSubentriesSharingSameKey(const Archive::DirectoryEntry &directoryEntry,
													  const Archive::DirectorySubEntry &directorySubEntry) {
	for (uint i = 0; i < directoryEntry.subentries.size(); i++) {
		const Archive::DirectorySubEntry &otherSubEntry = directoryEntry.subentries[i];
		if (otherSubEntry.type == directorySubEntry.type && otherSubEntry.face == directorySubEntry.face && otherSubEntry.offset != directorySubEntry.offset) {
			return true;
		}
	}

	return false;
}

Common::String ResourceLoader::computeExtractedFileName(const Archive::DirectoryEntry &directoryEntry,
														const Archive::DirectorySubEntry &directorySubEntry,
														bool multipleSubEntriesWithSameKey) {
	return computeExtractedFileName(directoryEntry, directorySubEntry, multipleSubEntriesWithSameKey, "jpg", "data", "dds");
}

Common::String ResourceLoader::computeExtractedFileName(const Archive::DirectoryEntry &directoryEntry,
														const Archive::DirectorySubEntry &directorySubEntry,
														bool multipleSubEntriesWithSameKey,
														const char *imagesFileExtension,
														const char *cursorFileExtension,
														const char *moddedImagesFileExtension) {
	bool printFace = true;
	Common::String extension;
	switch (directorySubEntry.type) {
	case Archive::kNumMetadata:
	case Archive::kTextMetadata:
		return ""; // These types are pure metadata and can't be extracted
	case Archive::kCubeFace:
	case Archive::kFrame:
	case Archive::kLocalizedFrame:
	case Archive::kSpotItem:
	case Archive::kLocalizedSpotItem:
		extension = imagesFileExtension;
		break;
	case Archive::kModdedCubeFace:
	case Archive::kModdedFrame:
	case Archive::kModdedSpotItem:
		extension = moddedImagesFileExtension;
		break;
	case Archive::kWaterEffectMask:
		extension = "water";
		break;
	case Archive::kLavaEffectMask:
		extension = "lava";
		break;
	case Archive::kMagneticEffectMask:
		extension = "magnet";
		break;
	case Archive::kShieldEffectMask:
		extension = "shield";
		break;
	case Archive::kMovie:
	case Archive::kStillMovie:
	case Archive::kDialogMovie:
	case Archive::kMultitrackMovie:
	case Archive::kModdedMovie:
		printFace = false;
		extension = "bik";
		break;
	case Archive::kRawData:
		printFace = false;
		extension = cursorFileExtension;
		break;
	case Archive::kModdedRawData:
		printFace = false;
		extension = moddedImagesFileExtension;
		break;
	default:
		extension = Common::String::format("%d", directorySubEntry.type);
		break;
	}

	if (printFace && multipleSubEntriesWithSameKey) {
		return Common::String::format("dump/%s-%d-%d-%d.%s", directoryEntry.roomName.c_str(), directoryEntry.index,
									  directorySubEntry.face, directorySubEntry.offset, extension.c_str());
	}

	if (printFace && !multipleSubEntriesWithSameKey) {
		return Common::String::format("dump/%s-%d-%d.%s", directoryEntry.roomName.c_str(), directoryEntry.index,
									  directorySubEntry.face, extension.c_str());
	}

	if (multipleSubEntriesWithSameKey) {
		return Common::String::format("dump/%s-%d-%d.%s", directoryEntry.roomName.c_str(), directoryEntry.index,
									  directorySubEntry.offset, extension.c_str());
	}

	return Common::String::format("dump/%s-%d.%s", directoryEntry.roomName.c_str(), directoryEntry.index,
								  extension.c_str());
}

TexDecoder::~TexDecoder() {
	_outputSurface.free();
}

bool TexDecoder::loadStream(Common::SeekableReadStream &stream, const Common::String &name) {
	uint32 magic = stream.readUint32LE();
	if (magic != MKTAG('.', 'T', 'E', 'X')) {
		warning("Invalid texture format for '%s'", name.c_str());
		return false;
	}

	stream.readUint32LE(); // unk 1
	uint32 width = stream.readUint32LE();
	uint32 height = stream.readUint32LE();
	stream.readUint32LE(); // unk 2
	stream.readUint32LE(); // unk 3

#ifdef SCUMM_BIG_ENDIAN
	Graphics::PixelFormat onDiskFormat = Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 24, 16, 8);
#else
	Graphics::PixelFormat onDiskFormat = Graphics::PixelFormat(4, 8, 8, 8, 8, 8, 16, 24, 0);
#endif

	_outputSurface.create(width, height, onDiskFormat);
	stream.read(_outputSurface.getPixels(), height * _outputSurface.pitch);

	_outputSurface.convertToInPlace(Texture::getRGBAPixelFormat());

	return true;
}

static Common::SeekableReadStream *openFile(const Common::String &filename) {
	debugC(kDebugModding, "Attempting to load external file '%s'", filename.c_str());

	// FIXME: FSNode::createReadStream should not print a warning when attempting to open non existing files
	Common::FSNode fsnode = Common::FSNode(Common::Path(filename));
	if (!fsnode.exists()) {
		return nullptr;
	}

	Common::SeekableReadStream *externalStream = fsnode.createReadStream();
	if (externalStream) {
		debugC(kDebugModding, "Loaded external file '%s'", filename.c_str());
	}

	return externalStream;
}

TextureLoader::TextureLoader(Renderer &renderer) : _renderer(renderer),
												   _loadExternalFiles(ConfMan.getBool("enable_assets_mod")) {
}

Graphics::Surface *TextureLoader::loadSurface(const ResourceDescription &resource, TextureLoader::ImageFormat defaultImageFormat) {
	ImageFormat imageFormat = kImageFormatUnknown;
	Common::SeekableReadStream *imageStream = nullptr;
	Common::String name = Common::String::format("%s-%d-%d", resource.getRoom().c_str(), resource.getIndex(), resource.getFace());

	if (_loadExternalFiles) {
		bool multipleSubEntriesWithSameKey = ResourceLoader::checkForSubentriesSharingSameKey(resource.getDirectoryEntry(), resource.getDirectorySubEntry());
		name = ResourceLoader::computeExtractedFileName(resource.getDirectoryEntry(), resource.getDirectorySubEntry(), multipleSubEntriesWithSameKey, "dds", "dds", "dds");
		imageStream = openFile(name);
		if (imageStream) {
			imageFormat = kImageFormatDDS;
		}

		if (!imageStream) {
			name = ResourceLoader::computeExtractedFileName(resource.getDirectoryEntry(), resource.getDirectorySubEntry(), multipleSubEntriesWithSameKey, "png", "png", "png");
			imageStream = openFile(name);
			if (imageStream) {
				imageFormat = kImageFormatPNG;
			}
		}

		if (!imageStream) {
			name = ResourceLoader::computeExtractedFileName(resource.getDirectoryEntry(), resource.getDirectorySubEntry(), multipleSubEntriesWithSameKey, "jpg", "jpg", "jpg");
			imageStream = openFile(name);
			if (imageStream) {
				imageFormat = kImageFormatJPEG;
			}
		}
	}

	if (!imageStream) {
		imageStream = resource.getData();
		if (resource.getType() == Archive::kModdedCubeFace || resource.getType() == Archive::kModdedFrame || resource.getType() == Archive::kModdedSpotItem || resource.getType() == Archive::kModdedRawData) {
			imageFormat = kImageFormatDDS;
		} else {
			// Detect format from magic number to handle LZO-decompressed DDS data
			uint32 magic = imageStream->readUint32BE();
			imageStream->seek(0);
			if (magic == MKTAG('D', 'D', 'S', ' ')) {
				imageFormat = kImageFormatDDS;
			} else {
				imageFormat = defaultImageFormat;
			}
		}
	}

	Graphics::Surface *surface = new Graphics::Surface();
	switch (imageFormat) {
	case kImageFormatJPEG: {
		Image::JPEGDecoder jpeg;
		jpeg.setOutputPixelFormat(Texture::getRGBAPixelFormat());

		if (!jpeg.loadStream(*imageStream)) {
			error("Failed to decode JPEG %s", name.c_str());
		}

		const Graphics::Surface *bitmap = jpeg.getSurface();
		assert(bitmap->format == Texture::getRGBAPixelFormat());
		surface->copyFrom(*bitmap);
		break;
	}
	case kImageFormatPNG: {
		Image::PNGDecoder decoder;

		if (!decoder.loadStream(*imageStream)) {
			error("Failed to decode PNG %s", name.c_str());
		}
		surface->copyFrom(*decoder.getSurface());
		break;
	}
	case kImageFormatDDS: {
		DDS decoder;

		if (!decoder.load(*imageStream, name)) {
			error("Failed to decode DDS %s", name.c_str());
		}

		switch (decoder.dataFormat()) {
		case DDS::kDataFormatMipMaps:
			surface->copyFrom(decoder.getMipMaps()[0]);
			break;
		case DDS::kDataFormatRawBC1Unorm: {
			Graphics::Surface *decompressed = decompressDXT1(decoder.rawData(), decoder.rawDataSize(), decoder.width(), decoder.height());
			surface->copyFrom(*decompressed);
			decompressed->free();
			delete decompressed;
			break;
		}
		case DDS::kDataFormatRawBC2Unorm: {
			Graphics::Surface *decompressed = decompressDXT3(decoder.rawData(), decoder.rawDataSize(), decoder.width(), decoder.height());
			surface->copyFrom(*decompressed);
			decompressed->free();
			delete decompressed;
			break;
		}
		case DDS::kDataFormatRawBC3Unorm: {
			Graphics::Surface *decompressed = decompressDXT5(decoder.rawData(), decoder.rawDataSize(), decoder.width(), decoder.height());
			surface->copyFrom(*decompressed);
			decompressed->free();
			delete decompressed;
			break;
		}
		case DDS::kDataFormatRawBC7Unorm:
			error("BC7 decompression not implemented for %s", name.c_str());
			break;
		default:
			error("Unknown DDS data format for %s", name.c_str());
		}
		break;
	}
	case kImageFormatTEX: {
		TexDecoder decoder;

		if (!decoder.loadStream(*imageStream, name)) {
			error("Failed to decode TEX %s", name.c_str());
		}
		surface->copyFrom(*decoder.getSurface());
		break;
	}
	case kImageFormatBMP: {
		Image::BitmapDecoder decoder;
		if (!decoder.loadStream(*imageStream)) {
			error("Failed to decode BMP %s", name.c_str());
		}

		const Graphics::Surface *surfaceBGRA = decoder.getSurface();
		Graphics::Surface *surfaceRGBA = surfaceBGRA->convertTo(Texture::getRGBAPixelFormat());

		// Apply the colorkey for transparency
		for (int y = 0; y < surfaceRGBA->h; y++) {
			byte *pixels = (byte *)(surfaceRGBA->getBasePtr(0, y));
			for (int x = 0; x < surfaceRGBA->w; x++) {
				byte *r = pixels + 0;
				byte *g = pixels + 1;
				byte *b = pixels + 2;
				byte *a = pixels + 3;

				if (*r == 0 && *g == 0xFF && *b == 0 && *a == 0xFF) {
					*g = 0;
					*a = 0;
				}

				pixels += 4;
			}
		}

		surface->free();
		delete surface;
		surface = surfaceRGBA;
		break;
	}
	default:
		error("Unknown image format %d", imageFormat);
	}
	delete imageStream;
	return surface;
}

Texture *TextureLoader::load(const ResourceDescription &resource, TextureLoader::ImageFormat defaultImageFormat) {
	// For DDS files, try to use GPU-native compression first
	if (_renderer.supportsCompressedTextures()) {
		ImageFormat imageFormat = defaultImageFormat;

		// Determine if this is a DDS resource
		if (resource.getType() == Archive::kModdedCubeFace ||
		    resource.getType() == Archive::kModdedFrame ||
		    resource.getType() == Archive::kModdedSpotItem ||
		    resource.getType() == Archive::kModdedRawData) {
			imageFormat = kImageFormatDDS;
		}

		if (imageFormat == kImageFormatDDS) {
			Common::SeekableReadStream *stream = resource.getData();
			Common::String name = Common::String::format("%s-%d-%d", resource.getRoom().c_str(), resource.getIndex(), resource.getFace());

			DDS dds;
			if (dds.load(*stream, name)) {
				delete stream;

				// Try GPU-native texture creation
				Texture *texture = _renderer.createTextureFromDDS(dds);
				if (texture) {
					return texture;
				}
				// Fall through to software decompression if GPU path failed
			} else {
				delete stream;
			}
		}
	}

	// Fallback to software decompression path
	Graphics::Surface *surface = loadSurface(resource, defaultImageFormat);
	Texture *texture = _renderer.createTexture2D(surface);

	surface->free();
	delete surface;
	return texture;
}

VideoLoader::VideoLoader() : _loadExternalFiles(ConfMan.getBool("enable_assets_mod")) {
}

Common::SeekableReadStream *VideoLoader::load(const ResourceDescription &resource) {
	Common::SeekableReadStream *binkStream = nullptr;
	if (_loadExternalFiles) {
		bool multipleSubEntriesWithSameKey = ResourceLoader::checkForSubentriesSharingSameKey(resource.getDirectoryEntry(), resource.getDirectorySubEntry());
		Common::String extractedFileName = ResourceLoader::computeExtractedFileName(resource.getDirectoryEntry(), resource.getDirectorySubEntry(), multipleSubEntriesWithSameKey);
		debugC(kDebugModding, "Attempting to load external file '%s'", extractedFileName.c_str());
		binkStream = SearchMan.createReadStreamForMember(Common::Path(extractedFileName));
		if (binkStream) {
			debugC(kDebugModding, "Loaded external file '%s'", extractedFileName.c_str());
		}
	}

	if (!binkStream) {
		binkStream = resource.getData();
	}

	return binkStream;
}

} // End of namespace Myst3
