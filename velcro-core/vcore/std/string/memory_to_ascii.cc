#include <vcore/std/string/memory_to_ascii.h>

namespace VStd
{
    namespace MemoryToASCII
    {
        VStd::string ToString(const void* memoryAddrs, VStd::size_t dataSize, VStd::size_t maxShowSize, VStd::size_t dataWidth/*=16*/, Options format/*=Options::Default*/)
        {
            VStd::string output;

            if ((memoryAddrs != nullptr) && (dataSize > 0))
            {
                const V::u8 *data = reinterpret_cast<const V::u8*>(memoryAddrs);

                if (static_cast<unsigned>(format) != 0)
                {
                    output.reserve(8162);

                    bool showHeader = static_cast<unsigned>(format) & static_cast<unsigned>(Options::Header) ? true : false;
                    bool showOffset = static_cast<unsigned>(format) & static_cast<unsigned>(Options::Offset) ? true : false;
                    bool showBinary = static_cast<unsigned>(format) & static_cast<unsigned>(Options::Binary) ? true : false;
                    bool showASCII = static_cast<unsigned>(format) & static_cast<unsigned>(Options::ASCII) ? true : false;
                    bool showInfo = static_cast<unsigned>(format) & static_cast<unsigned>(Options::Info) ? true : false;

                    // Because of the auto formatting for the headers, the min width is 3
                    if (dataWidth < 3)
                    {
                        dataWidth = 3;
                    }

                    if (showHeader)
                    {
                        VStd::string line1;
                        VStd::string line2;
                        line1.reserve(1024);
                        line2.reserve(1024);

                        if (showOffset)
                        {
                            line1 += "Offset";
                            line2 += "------";

                            if (showBinary || showASCII)
                            {
                                line1 += " ";
                                line2 += " ";
                            }
                        }

                        if (showBinary)
                        {
                            static const char *kHeaderName = "Data";
                            static VStd::size_t kHeaderNameSize = 4;

                            VStd::size_t lineLength = (dataWidth * 3) - 1;
                            VStd::size_t numPreSpaces = (lineLength - kHeaderNameSize) / 2;
                            VStd::size_t numPostSpaces = lineLength - numPreSpaces - kHeaderNameSize;

                            line1 += VStd::string(numPreSpaces, ' ') + kHeaderName + VStd::string(numPostSpaces, ' ');
                            //line2 += VStd::string(lineLength, '-');
                            for(size_t i=0; i<dataWidth; i++)
                            {
                                if (i > 0)
                                {
                                    line2 += "-";
                                }

                                line2 += VStd::string::format("%02zx", i);
                            }

                            if (showASCII)
                            {
                                line1 += " ";
                                line2 += " ";
                            }
                        }

                        if (showASCII)
                        {
                            static const char *kHeaderName = "ASCII";
                            static VStd::size_t kHeaderNameSize = 5;

                            VStd::size_t numPreSpaces = (dataWidth - kHeaderNameSize) / 2;
                            VStd::size_t numPostSpaces = dataWidth - numPreSpaces - kHeaderNameSize;

                            line1 += VStd::string(numPreSpaces, ' ') + kHeaderName + VStd::string(numPostSpaces, ' ');
                            line2 += VStd::string(dataWidth, '-');
                        }

                        if (showInfo)
                        {
                            output += VStd::string::format("Address: 0x%p Data Size:%zu Max Size:%zu\n", data, dataSize, maxShowSize);
                        }

                        output += line1 + "\n";
                        output += line2 + "\n";
                    }

                    VStd::size_t offset = 0;
                    VStd::size_t maxSize = dataSize > maxShowSize ? maxShowSize : dataSize;

                    while (offset < maxSize)
                    {
                        if (showOffset)
                        {
                            output += VStd::string::format("%06zx", offset);

                            if (showBinary || showASCII)
                            {
                                output += " ";
                            }
                        }

                        if (showBinary)
                        {
                            VStd::string binLine;
                            binLine.reserve((dataWidth * 3) * 2);

                            for (VStd::size_t index = 0; index < dataWidth; index++)
                            {
                                if (!binLine.empty())
                                {
                                    binLine += " ";
                                }

                                if ((offset + index) < maxSize)
                                {
                                    binLine += VStd::string::format("%02x", data[offset + index]);
                                }
                                else
                                {
                                    binLine += "  ";
                                }
                            }

                            output += binLine;

                            if (showASCII)
                            {
                                output += " ";
                            }
                        }

                        if (showASCII)
                        {
                            VStd::string asciiLine;
                            asciiLine.reserve(dataWidth * 2);

                            for (VStd::size_t index = 0; index < dataWidth; index++)
                            {
                                if ((offset + index) > maxSize)
                                {
                                    break;
                                }
                                else
                                {
                                    char value = static_cast<char>(data[offset + index]);

                                    if ((value < 32) || (value > 127))
                                        value = ' ';

                                    asciiLine += value;
                                }
                            }

                            output += asciiLine;
                        }

                        output += "\n";
                        offset += dataWidth;
                    }
                }
            }

            return output;
        }
    } // namespace MemoryToASCII
} // namespace VStd
