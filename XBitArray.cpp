#include <XVMem_platform.h>
#include <locale>
#include <string>

#include <unordered_map>
#include <XBitArray.h>

XBitArray255::XBitArray255() {
	this->directory = 0;
	for (auto a = 0; a < 16; a++)
		array[a] = 0;
}

XBitArray255::XBitArray255(UINT16 compacted[]) {
	this->directory = 0;
	BYTE cnt = 0;
	BYTE a = 0;
	UINT16 bit = 1;

	do {
		if ((compacted[0] & bit) != 0)
			array[a] = compacted[++cnt];
		else
			array[a] = 0;
		a++;
		bit <<= 1;
	}	while (bit != 0);

	for (/**/; a < 16; a++)
		array[a] = 0;
}

XBitArray255::XBitArray255(BYTE items[]) {
	this->directory = 0;
	for (auto a = 0; a < 16; a++)
		array[a] = 0;
	if (items != NULL) {
		for (BYTE* position = items; *position; position++)
			SetBit(*position);
	}
}

XBitArray255::XBitArray255(bool positionalBits[], BYTE cnt) {
	this->directory = 0;
	for (auto a = 0; a < 16; a++)
		array[a] = 0;
	if (positionalBits != NULL && cnt > 0 && cnt <= 255) {
		BYTE i = 1;
		for (bool* position = positionalBits; i <= 255; position++, i++)
			if (*position)
				SetBit(i);
			else
				UnsetBit(i);
	}
}

bool XBitArray255::SetBit(BYTE item) {
	if (item > 0) {
		BYTE index = (item-1) / 16;
		BYTE local = (item-1) % 16;

		UINT16 dir = 1;
		if (index > 0)
			dir <<= index;
		this->directory |= dir;

		UINT16 loc = 1;
		if (local > 0)
			loc <<= local;
		this->array[index] |= loc;

		return true;
	}
	return false;
}

bool XBitArray255::UnsetBit(BYTE item) {
	if (item > 0) {
		BYTE index = (item - 1) / 16;
		BYTE local = (item - 1) % 16;

		UINT16 dir = 1;
		if (index > 0)
			dir <<= index;
		this->directory |= dir;

		UINT16 loc = 1;
		if (local > 0)
			loc <<= local;
		this->array[index] ^= loc;

		return true;
	}
	return false;
}

UINT16* XBitArray255::CreateCompactBitArray() {	// up to size=17 UINT16[]	// most compact
	int countSegments = 0;
	for (auto s = 0; s < 16; s++)
		if (this->array[s] != 0)
			countSegments++;

	auto result = (UINT16*) malloc((1 + countSegments) * sizeof(UINT16));
	result[0] = this->directory;
	BYTE cnt = 1;
	for (auto index = 0; index < 16; index++)
		if (this->array[index] != 0)
			result[cnt++] = this->array[index];

	return result;
}

BYTE XBitArray255::GetCompactBitArray(UINT16 result[], BYTE maxCnt) {			// up to size=255 BYTE[]	// much less compact
	int countSegments = 0;
	for (auto s = 0; s < 16; s++)
		if (this->array[s] != 0)
			countSegments++;

	result[0] = this->directory;
	if (result[0] == 0)
		return 1;

	BYTE cnt = 0;
	for (auto index = 0; index < 16; index++)
		if (this->array[index] != 0)
			result[++cnt] = this->array[index];

	return countSegments + 1;
}

BYTE* XBitArray255::CreateByteArray() {			// up to size=255 BYTE[]	// much less compact
	BYTE cnt = 0;
	for (auto index = 0; index < 16; index++)
		if (this->array[index] != 0)
			cnt += CountBits(this->array[index]);

	auto result = (BYTE*)malloc(1 + cnt * sizeof(BYTE));
	result[cnt] = 0;
	BYTE baseline = 0;
	BYTE idx = 0;
	for (auto index = 0; index < 16; index++) {
		for (UINT16 bit = 0x1; bit != 0; bit <<= 1) {
			if ((this->array[index] & bit) != 0) {
				BYTE value = 1 + baseline;
				result[idx++] = value;
			}
			baseline++;
		}
	}
	return result;
}

BYTE XBitArray255::GetBoolArray(bool result[], BYTE maxCnt) {// up to size=255 bool[]	// least compact
	BYTE cnt = 0;
	BYTE baseline = 0;
	for (auto index = 0; index < 16; index++) {
		if (this->array[index] != 0) {
			for (UINT16 bit = 1; bit != 0; bit <<= 1) {
				bool on = ((this->array[index] & bit) != 0);
				if (on)
					cnt = baseline + 1;
				if (baseline < maxCnt) {
					array[baseline] = on;
				}
			}
			baseline++;
		}
	}
	return cnt;
}