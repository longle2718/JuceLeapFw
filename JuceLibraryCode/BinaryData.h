/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#ifndef BINARYDATA_H_100921108_INCLUDED
#define BINARYDATA_H_100921108_INCLUDED

namespace BinaryData
{
    extern const char*   juce_icon_png;
    const int            juce_icon_pngSize = 19826;

    extern const char*   brushed_aluminium_png;
    const int            brushed_aluminium_pngSize = 14724;

    extern const char*   portmeirion_jpg;
    const int            portmeirion_jpgSize = 145904;

    extern const char*   teapot_obj;
    const int            teapot_objSize = 95000;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 4;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}

#endif
