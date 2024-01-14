//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 - present Mikael Sundell.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>

// imath
#include <Imath/ImathMatrix.h>
#include <Imath/ImathVec.h>

// openimageio
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/argparse.h>
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/sysutil.h>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using namespace OIIO;

// prints
template <typename T>
static void
print_info(std::string param, const T& value)
{
    std::cout << "info: " << param << value << std::endl;
}

template <typename T>
static void
print_warning(std::string param, const T& value)
{
    std::cout << "warning: " << param << value << std::endl;
}

template <typename T>
static void
print_error(std::string param, const T& value)
{
    std::cerr << "error: " << param << value << std::endl;
}

// overlay tool
struct OverlayTool
{
    bool help = false;
    bool verbose = false;
    std::string outputfile;
    float aspectratio = 1.5f;
    float scale = 0.5f;
    Imath::Vec3<float> color = Imath::Vec3<float>(1.0f, 1.0f, 1.0f);
    Imath::Vec2<int> size = Imath::Vec2<int>(1024, 1024);
    bool centerpoint = false;
    bool symmetrygrid = false;
    bool label = false;
    bool debug;
    int code = EXIT_SUCCESS;
};

static OverlayTool tool;

static int
set_outputfile(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.outputfile = argv[1];
    return 0;
}

// --aspectratio
static int
set_aspectratio(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    std::istringstream iss(argv[1]);
    iss >> tool.aspectratio;
    if (iss.fail()) {
        print_error("could not parse aspect ratio from string: ", argv[1]);
        return 1;
    } else {
        return 0;
    }
}

// --scale
static int
set_scale(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    std::istringstream iss(argv[1]);
    iss >> tool.scale;
    if (iss.fail()) {
        print_error("could not parse scale from string: ", argv[1]);
        return 1;
    } else {
        return 0;
    }
}

// --color
static int
set_color(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    std::istringstream iss(argv[1]);
    iss >> tool.color.x;
    iss.ignore(); // Ignore the comma
    iss >> tool.color.y;
    iss.ignore(); // Ignore the comma
    iss >> tool.color.z;
    if (iss.fail()) {
        print_error("could not parse color from string: ", argv[1]);
        return 1;
    } else {
        return 0;
    }
}

// --size
static int
set_size(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    std::istringstream iss(argv[1]);
    iss >> tool.size.x;
    iss.ignore(); // Ignore the comma
    iss >> tool.size.y;
    if (iss.fail()) {
        print_error("could not parse size from string: ", argv[1]);
        return 1;
    } else {
        return 0;
    }
}

// --help
static void
print_help(ArgParse& ap)
{
    ap.print_help();
}

// utils
void renderBoxByThickness(ImageBuf& imagebuf, ROI roi, Imath::Vec3<float> color, int thickness) {

    for (int t=0; t<thickness; t++) {
        ImageBufAlgo::render_box(
            imagebuf, roi.xbegin + t, roi.ybegin + t, roi.xend - t - 1, roi.yend - t - 1,
            { color.x, color.y, color.z, 1.0f }
        );
        ImageBufAlgo::render_box(
            imagebuf, roi.xbegin - t, roi.ybegin - t, roi.xend + t - 1, roi.yend + t - 1,
            { color.x, color.y, color.z, 1.0f }
        );
    }
}

void renderLineByPattern(ImageBuf& imagebuf, ROI roi, Imath::Vec3<float> color, int dot_interval) {

    float length = std::sqrt(std::pow(roi.xend - roi.xbegin, 2) + std::pow(roi.yend - roi.ybegin, 2));
    int dots = std::round(length / dot_interval);
    for (int i = 0; i < dots; ++i) {
        if (i % 2 == 0) {
            float start = static_cast<float>(i) / dots;
            float end = static_cast<float>(i + 1) / dots;
            int xbegin = roi.xbegin + std::round((roi.xend - roi.xbegin) * start);
            int ybegin = roi.ybegin + std::round((roi.yend - roi.ybegin) * start);
            int xend = roi.xbegin + std::round((roi.xend - roi.xbegin) * end);
            int yend = roi.ybegin + std::round((roi.yend - roi.ybegin) * end);

            ImageBufAlgo::render_line(
                imagebuf,
                xbegin,
                ybegin,
                xend,
                yend,
                { color.x, color.y, color.z, 1.0f }
            );
        }
    }
}

// utils -- region of interest
ROI scaleBy(ROI roi, float sx, float sy)
{
    int cx = (roi.xbegin + roi.xend) / 2;
    int cy = (roi.ybegin + roi.yend) / 2;

    int width = roi.width();
    int height = roi.height();
    int swidth = width * sx;
    int sheight = height * sy;

    int dx = (swidth - width) / 2;
    int dy = (sheight - height) / 2;
    
    ROI sroi = roi;
    {
        sroi.xbegin -= dx;
        sroi.xend = sroi.xbegin + swidth;
        sroi.ybegin -= dy;
        sroi.yend = sroi.ybegin + sheight;
    }
    return sroi;
}

ROI aspectRatioBy(ROI roi, float aspectRatio)
{
    float ar = (float)roi.width() / roi.height();
    if (ar != aspectRatio)
    {
        int awidth, aheight;
        int hdiff = 0;
        
        if (ar < aspectRatio) {
            aheight = (int)(roi.width() / aspectRatio);
            hdiff = aheight - roi.height();
        } else {
            aheight = (int)(roi.width() / aspectRatio);
            hdiff = aheight - roi.height();
        }
        
        int cx = roi.xbegin + roi.width() / 2;
        int cy = roi.ybegin + roi.height() / 2;

        ROI arroi = roi;
        {
            arroi.xbegin = cx - arroi.width() / 2;
            arroi.xend = arroi.xbegin + arroi.width();
            arroi.ybegin = cy - arroi.height() / 2 - hdiff / 2;
            arroi.yend = arroi.ybegin + aheight;
        }
        return arroi;
    }
    return roi;
}

// utils -- trigonometry
float radiansBy90()
{
    return M_PI / 2.0;
}

float degreesByRadians(float radians)
{
    return radians * 180.0f / M_PI;
}

// main
int 
main( int argc, const char * argv[])
{
    // Helpful for debugging to make sure that any crashes dump a stack
    // trace.
    Sysutil::setup_crash_stacktrace("stdout");

    Filesystem::convert_native_arguments(argc, (const char**)argv);
    ArgParse ap;

    ap.intro("overlaytool -- a utility for creating overlay images\n");
    ap.usage("overlaytool [options] ...")
      .add_help(false)
      .exit_on_error(true);
    
    ap.separator("General flags:");
    ap.arg("--help", &tool.help)
      .help("Print help message");
    
    ap.arg("-v", &tool.verbose)
      .help("Verbose status messages");
    
    ap.arg("-d", &tool.debug)
      .help("Debug status messages");
    
    ap.separator("Input flags:");
    ap.arg("--centerpoint", &tool.centerpoint)
      .help("Use centerpoint for overlay");
    
    ap.arg("--symmetrygrid", &tool.symmetrygrid)
      .help("Use symmetry grid for overlay");
    
    ap.arg("--label", &tool.label)
      .help("Use label for overlay");
       
    ap.arg("--aspectratio %s:ASPECTRATIO")
      .help("Set aspectratio (default:1.5)")
      .action(set_aspectratio);
    
    ap.arg("--scale %s:SCALE")
      .help("Set scale (default: 0.5)")
      .action(set_scale);
    
    ap.arg("--color %s:COLOR")
      .help("Set color (default: 1.0, 1.0, 1.0)")
      .action(set_color);
    
    ap.arg("--size %s:SIZE")
      .help("Set size (default: 1024, 1024)")
      .action(set_size);
    
    ap.separator("Output flags:");
    ap.arg("--outputfile %s:OUTPUTFILE")
      .help("Set output file")
      .action(set_outputfile);
    
    // clang-format on
    if (ap.parse_args(argc, (const char**)argv) < 0) {
        std::cerr << "error: " << ap.geterror() << std::endl;
        print_help(ap);
        ap.abort();
        return EXIT_FAILURE;
    }
    if (ap["help"].get<int>()) {
        print_help(ap);
        ap.abort();
        return EXIT_SUCCESS;
    }
    
    if (!tool.outputfile.size()) {
        std::cerr << "error: must have output file parameter\n";
        ap.briefusage();
        ap.abort();
        return EXIT_FAILURE;
    }
    if (argc <= 1) {
        ap.briefusage();
        std::cout << "\nFor detailed help: overlaytool --help\n";
        return EXIT_FAILURE;
    }

    // overlay program
    std::cout << "overlaytool -- a utility for creating overlay images" << std::endl;

    print_info("Writing overlay file: ", tool.outputfile);
    ImageSpec spec(tool.size.x, tool.size.y, 4, TypeDesc::FLOAT);
    ImageBuf imagebuf(spec);
    
    // overlay
    ROI roi(0, tool.size.x, 0, tool.size.y);

    renderBoxByThickness(
        imagebuf,
        roi,
        tool.color,
        2
    );
    
    // aspect ratio
    ROI arroi = scaleBy(aspectRatioBy(roi, tool.aspectratio), tool.scale, tool.scale);
    renderBoxByThickness(
        imagebuf,
        arroi,
        tool.color,
        2
    );
    
    // center point
    if (tool.centerpoint) {
        
        Imath::Vec2<float> center(
            (arroi.xbegin + arroi.xend) / 2,
            (arroi.ybegin + arroi.yend) / 2
        );
        int cross = 0;
        if (arroi.width() > arroi.height()) {
            cross = arroi.width() * 0.05;
        } else {
            cross = arroi.height() * 0.05;
        }
        
        int xbegin = center.x - (cross / 2);
        int xend = xbegin + cross - 1;
        ImageBufAlgo::render_line(
            imagebuf,
            xbegin,
            center.y,
            xend,
            center.y,
            { tool.color.x, tool.color.y, tool.color.z, 1.0f }
        );
        
        int ybegin = center.y - (cross / 2);
        int yend = ybegin + cross - 1;
        ImageBufAlgo::render_line(
            imagebuf,
            center.x,
            ybegin,
            center.x,
            yend,
            { tool.color.x, tool.color.y, tool.color.z, 1.0f }
        );
    }
    
    // symmetry grid
    if (tool.symmetrygrid) {
        
        // baroque diagonal
        ImageBufAlgo::render_line(
            imagebuf,
            arroi.xbegin,
            arroi.yend - 1,
            arroi.xend - 1,
            arroi.ybegin,
            { tool.color.x, tool.color.y, tool.color.z, 1.0f }
        );
        
        // diagonals
        {
            ROI diagonal(
                arroi.xbegin,
                arroi.xend - 1,
                arroi.ybegin,
                arroi.yend - 1
            );

            ImageBufAlgo::render_line(
                imagebuf,
                diagonal.xbegin,
                diagonal.ybegin,
                diagonal.xend,
                diagonal.yend,
                { tool.color.x, tool.color.y, tool.color.z, 1.0f }
            );
            
            // reciprocals
            {
                Imath::Vec2<float> d(
                    arroi.xend - arroi.xbegin - 1,
                    arroi.yend - arroi.ybegin - 1
                );
                float angle = radiansBy90() - std::atan(d.x / d.y);
                float length = d.y * std::tan(angle);
                float hypo = d.y * std::cos(angle);
                Imath::Vec2<float> cross(
                    hypo * std::sin(angle),
                    hypo * std::cos(angle)
                );
                                    
                // diagonals
                {
                    ImageBufAlgo::render_line(
                        imagebuf,
                        arroi.xbegin,
                        arroi.ybegin,
                        arroi.xbegin + length,
                        arroi.yend,
                        { tool.color.x, tool.color.y, tool.color.z, 1.0f }
                    );
                    
                    ImageBufAlgo::render_line(
                        imagebuf,
                        arroi.xbegin,
                        arroi.yend,
                        arroi.xbegin + length,
                        arroi.ybegin,
                        { tool.color.x, tool.color.y, tool.color.z, 1.0f }
                    );
                    
                    ImageBufAlgo::render_line(
                        imagebuf,
                        arroi.xend,
                        arroi.ybegin,
                        arroi.xend - length,
                        arroi.yend,
                        { tool.color.x, tool.color.y, tool.color.z, 1.0f }
                    );
                    
                    ImageBufAlgo::render_line(
                        imagebuf,
                        arroi.xend,
                        arroi.yend,
                        arroi.xend - length,
                        arroi.ybegin,
                        { tool.color.x, tool.color.y, tool.color.z, 1.0f }
                    );
                }
                
                // rectangles
                {
                    ImageBufAlgo::render_line(
                        imagebuf,
                        arroi.xbegin + cross.x,
                        arroi.ybegin,
                        arroi.xbegin + cross.x,
                        arroi.yend,
                        { tool.color.x, tool.color.y, tool.color.z, 1.0f }
                    );
                    
                    ImageBufAlgo::render_line(
                        imagebuf,
                        arroi.xend - cross.x,
                        arroi.ybegin,
                        arroi.xend - cross.x,
                        arroi.yend,
                        { tool.color.x, tool.color.y, tool.color.z, 1.0f }
                    );
                    
                    ImageBufAlgo::render_line(
                        imagebuf,
                        arroi.xbegin,
                        arroi.yend - cross.y,
                        arroi.xend,
                        arroi.yend - cross.y,
                        { tool.color.x, tool.color.y, tool.color.z, 1.0f }
                    );
                    
                    ImageBufAlgo::render_line(
                        imagebuf,
                        arroi.xbegin,
                        arroi.ybegin + cross.y,
                        arroi.xend,
                        arroi.ybegin + cross.y,
                        { tool.color.x, tool.color.y, tool.color.z, 1.0f }
                    );
                }
                
                // centers
                {
                    renderLineByPattern(
                        imagebuf,
                        ROI(
                            arroi.xbegin + length,
                            arroi.xbegin + length,
                            arroi.ybegin,
                            arroi.yend
                        ),
                        tool.color,
                        5
                    );
                    
                    renderLineByPattern(
                        imagebuf,
                        ROI(
                            arroi.xend - length,
                            arroi.xend - length,
                            arroi.ybegin,
                            arroi.yend
                        ),
                        tool.color,
                        5
                    );
                }
            }
        }
    }
    
    // label
    if (tool.label) {
        
        // overlay
        {
            std::ostringstream oss;
            oss << "size: "
                << tool.size.x
                << ", "
                << tool.size.y
                << " "
                << "aspect ratio: "
                << tool.aspectratio;

            ImageBufAlgo::render_text(
                imagebuf,
                roi.xbegin + roi.width() * 0.01,
                roi.yend - roi.width() * 0.01,
                oss.str(),
                12,
                "../Roboto.ttf",
                { tool.color.x, tool.color.y, tool.color.z, 1.0f },
                ImageBufAlgo::TextAlignX::Left,
                ImageBufAlgo::TextAlignY::Baseline
            );
        }
        
        // aspect ratio
        {
            std::ostringstream oss;
            oss << "size: "
                << arroi.width()
                << ", "
                << arroi.height()
                << " "
                << "scale: "
                << tool.scale;

            ImageBufAlgo::render_text(
                imagebuf,
                arroi.xbegin + arroi.width() * 0.01,
                arroi.yend + arroi.width() * 0.01,
                oss.str(),
                12,
                "../Roboto.ttf",
                { tool.color.x, tool.color.y, tool.color.z, 1.0f },
                ImageBufAlgo::TextAlignX::Left,
                ImageBufAlgo::TextAlignY::Top
            );
        }
    }
    
    if (!imagebuf.write(tool.outputfile)) {
        print_error("could not write output file", imagebuf.geterror());
    }
    return 0;
}
