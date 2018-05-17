/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef AES_H_INCLUDED
#define AES_H_INCLUDED

namespace hise {
using namespace juce;

class AES
{
public:

	AES(const String& keyToUse);

	~AES();

	void encrypt(MemoryBlock& mb);

	String encrypt(const String& stringToEncode);

	void decrypt(MemoryBlock& mb);

	String decrypt(const String& stringToDecode);

	void decrypt(AudioSampleBuffer& b);

	void encrypt(AudioSampleBuffer& b);

	static String createKey();

private:

	void prepare();

	AES_ctx context;
	uint8 key[16];
	Array<uint8_t> iv;

};


}

#endif