/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

// FIXME: This code is taken from SAGA and needs more work (e.g. setVolume).

// MIDI and digital music class

#include "audio/audiostream.h"
#include "audio/mididrv.h"
#include "audio/midiparser.h"
#include "common/config-manager.h"
#include "common/file.h"

#include "made/music.h"

namespace Made {

MusicPlayer::MusicPlayer() : _isGM(false) {
	MidiDriver::DeviceHandle dev = MidiDriver::detectDevice(MDT_MIDI | MDT_ADLIB | MDT_PREFER_GM);
	_nativeMT32 = ((MidiDriver::getMusicType(dev) == MT_MT32) || ConfMan.getBool("native_mt32"));
	//bool adlib = (MidiDriver::getMusicType(dev) == MT_ADLIB);

	_driver = MidiDriver::createMidi(dev);
	assert(_driver);
	if (_nativeMT32)
		_driver->property(MidiDriver::PROP_CHANNEL_MASK, 0x03FE);

	int ret = _driver->open();
	if (ret == 0) {
		if (_nativeMT32)
			_driver->sendMT32Reset();
		else
			_driver->sendGMReset();

		_driver->setTimerCallback(this, &timerCallback);
	}
}

void MusicPlayer::send(uint32 b) {
	if ((b & 0xF0) == 0xC0 && !_isGM && !_nativeMT32) {
		b = (b & 0xFFFF00FF) | MidiDriver::_mt32ToGm[(b >> 8) & 0xFF] << 8;
	}

	Audio::MidiPlayer::send(b);
}

void MusicPlayer::playXMIDI(GenericResource *midiResource, MusicFlags flags) {
	Common::StackLock lock(_mutex);

	if (_isPlaying)
		return;

	stop();

	// Load XMID resource data

	_isGM = true;

	MidiParser *parser = MidiParser::createParser_XMIDI();
	if (parser->loadMusic(midiResource->getData(), midiResource->getSize())) {
		parser->setTrack(0);
		parser->setMidiDriver(this);
		parser->setTimerRate(_driver->getBaseTempo());
		parser->property(MidiParser::mpCenterPitchWheelOnUnload, 1);
		parser->property(MidiParser::mpSendSustainOffOnNotesOff, 1);

		_parser = parser;

		setVolume(127);

		_isLooping = flags & MUSIC_LOOP;
		_isPlaying = true;
	} else {
		delete parser;
	}
}

void MusicPlayer::playSMF(GenericResource *midiResource, MusicFlags flags) {
	Common::StackLock lock(_mutex);

	if (_isPlaying)
		return;

	stop();

	// Load MIDI resource data

	_isGM = true;

	MidiParser *parser = MidiParser::createParser_SMF();
	if (parser->loadMusic(midiResource->getData(), midiResource->getSize())) {
		parser->setTrack(0);
		parser->setMidiDriver(this);
		parser->setTimerRate(_driver->getBaseTempo());
		parser->property(MidiParser::mpCenterPitchWheelOnUnload, 1);

		_parser = parser;

		setVolume(127);

		_isLooping = flags & MUSIC_LOOP;
		_isPlaying = true;
	} else {
		delete parser;
	}
}

void MusicPlayer::pause() {
	setVolume(-1);
	_isPlaying = false;
}

void MusicPlayer::resume() {
	setVolume(127);
	_isPlaying = true;
}

} // End of namespace Made
