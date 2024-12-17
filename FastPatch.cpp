#include "./FastPatch.h"

std::optional<std::vector<uint8_t>> FastPatch::stringToBytes(const std::string& byteString) {
	std::vector<uint8_t> bytes;
	std::istringstream stream(byteString);
	std::string byteStr;

	try {
		while (stream >> byteStr) {
			uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
			bytes.push_back(byte);
		}
	}
	catch (const std::exception&) {
		return std::nullopt;
	}

	return bytes;
}

std::optional<size_t> FastPatch::hexStringToAddress(const std::string& hexString) {
	size_t address = 0;
	try {
		address = std::stoull(hexString, nullptr, 16);
	}
	catch (const std::exception&) {
		return std::nullopt;
	}
	return address;
}

void FastPatch::patch(size_t address, std::vector<std::uint8_t> bytes) {
	auto size = bytes.size();
	DWORD old_prot;

	size_t absolute_address = base + address;

	VirtualProtect(reinterpret_cast<void*>(absolute_address), size, PAGE_EXECUTE_READWRITE, &old_prot);
	memcpy(reinterpret_cast<void*>(absolute_address), bytes.data(), size);
	VirtualProtect(reinterpret_cast<void*>(absolute_address), size, old_prot, &old_prot);
}

bool FastPatch::make(std::string addressString, std::string bytesString) {
	auto addressOpt = hexStringToAddress(addressString);
	if (!addressOpt.has_value()) return false;
	auto address = addressOpt.value();
	auto bytesOpt = stringToBytes(bytesString);
	if (!bytesOpt.has_value()) return false;
	auto bytes = bytesOpt.value();

	patch(address, bytes);
	return true;
}