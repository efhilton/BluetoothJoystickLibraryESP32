#pragma once

#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace efhilton
{
    class NetworkUtilities
    {
    public:
        /***
         * Given a set of mac address uint8s, build the MAC address string.
         * @param mac
         * @return
         */
        static std::string macToString(const uint8_t* mac)
        {
            std::ostringstream ss;
            ss << std::hex << std::uppercase << std::setfill('0');

            for (int i = 0; i < 6; i++)
            {
                ss << std::setw(2) << static_cast<int>(mac[5 - i]);
                if (i < 5)
                {
                    ss << ':';
                }
            }

            return ss.str();
        }

        static std::vector<uint8_t> uuidToReverseComponentOrder(const std::string& uuid)
        {
            std::string hexString(uuid);

            // Remove hyphens and convert to uppercase
            hexString.erase(std::remove(hexString.begin(), hexString.end(), '-'), hexString.end());
            std::transform(hexString.begin(), hexString.end(), hexString.begin(), ::toupper);

            // Convert to bytes
            std::vector<uint8_t> bytes;
            for (size_t i = 0; i < hexString.length(); i += 2)
            {
                std::string byteString = hexString.substr(i, 2);
                auto byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
                bytes.push_back(byte);
            }

            // Reverse the order
            std::reverse(bytes.begin(), bytes.end());
            return bytes;
        }

       static std::string bleUuid128ToGuid(const std::array<uint8_t, 16>& uuid)
        {
            std::ostringstream ss;
            ss << std::hex << std::setfill('0');

            // Section 1: 8 characters
            for (int i = 0; i < 4; ++i)
            {
                ss << std::setw(2) << static_cast<int>(uuid[i]);
            }
            ss << "-";

            // Section 2: 4 characters
            for (int i = 4; i < 6; ++i)
            {
                ss << std::setw(2) << static_cast<int>(uuid[i]);
            }
            ss << "-";

            // Section 3: 4 characters
            for (int i = 6; i < 8; ++i)
            {
                ss << std::setw(2) << static_cast<int>(uuid[i]);
            }
            ss << "-";

            // Section 4: 4 characters
            for (int i = 8; i < 10; ++i)
            {
                ss << std::setw(2) << static_cast<int>(uuid[i]);
            }
            ss << "-";

            // Section 5: 12 characters
            for (int i = 10; i < 16; ++i)
            {
                ss << std::setw(2) << static_cast<int>(uuid[i]);
            }

            return ss.str();
        }

        static std::array<uint8_t, 16> reverseUuid(const std::array<uint8_t, 16>& uuid)
        {
            auto reversedUuid = uuid;
            std::ranges::reverse(reversedUuid);
            return reversedUuid;
        }
    };
}
