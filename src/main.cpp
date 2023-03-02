#include <cstdio>
#include <cstdlib>
#include <limits>
#include <tuple>
#include <fstream>

#include <cmath>

#include <opencv2/opencv.hpp>

/* #include <proj_api.h> */

#include "defines.hpp"
#include "utils.hpp"

namespace {
    const char* STEP1_WIN_NAME = "Heightmap";
    const char* STEP2_WIN_NAME = "Edges";
    const int ZOOM = 1;
}


struct MouseProbe {
    cv::Mat & heightmap_8uc1_img_;
    cv::Mat & heightmap_show_8uc3_img_;
    cv::Mat & edgemap_8uc1_img_;

    MouseProbe(
            cv::Mat & heightmap_8uc1_img,
            cv::Mat & heightmap_show_8uc3_img,
            cv::Mat & edgemap_8uc1_img
       ) :
        heightmap_8uc1_img_(heightmap_8uc1_img),
        heightmap_show_8uc3_img_(heightmap_show_8uc3_img),
        edgemap_8uc1_img_(edgemap_8uc1_img) { }
};

// variables

// function declarations
void flood_fill(cv::Mat& src_img, cv::Mat& dst_img, int x, int y);


/**
 * Mouse clicking callback.
 */
void mouse_probe_handler(int event, int x, int y, int flags, void* param) {
    MouseProbe *probe = (MouseProbe*)param;

    switch (event) {

    case cv::EVENT_LBUTTONDOWN:
        printf("Clicked LEFT at: [%d, %d]\n", x, y);
        flood_fill(probe->edgemap_8uc1_img_, probe->heightmap_show_8uc3_img_, x, y);
        break;

    case cv::EVENT_RBUTTONDOWN:
        printf("Clicked RIGHT at: [%d, %d]\n", x, y);
        break;
    }
}


void create_windows(const int width, const int height) {
    cv::namedWindow(STEP1_WIN_NAME, 0);
    cv::namedWindow(STEP2_WIN_NAME, 0);

    cv::resizeWindow(STEP1_WIN_NAME, width*ZOOM, height*ZOOM);
    cv::resizeWindow(STEP2_WIN_NAME, width*ZOOM, height*ZOOM);

} // create_windows


/**
 * Perform flood fill from the specified point (x, y) for the neighborhood points if they contain the same value,
 * as the one that came in argument 'value'. Function recursicely call itself for its 4-neighborhood.
 * 
 * edgemap_8uc1_img - image, in which we perform flood filling
 * heightmap_show_8uc3_img - image, in which we display the filling
 * value - value, for which we'll perform flood filling
 */
void fill_step(
        cv::Mat& edgemap_8uc1_img,
        cv::Mat& heightmap_show_8uc3_img,
        const int x,
        const int y,
        const uchar value
    ) {
    int width, height;

} //fill_step


/**
 * Perform flood fill from the specified point (x, y). The function remembers the value at the coordinate (x, y)
 * and fill the neighborhood using 'fill_step' function so long as the value in the neighborhood points are the same.
 * Execute the fill on a temporary image to prevent the original image from being repainted.

 * edgemap_8uc1_img - image, in which we perform flood filling
 * heightmap_show_8uc3_img - image, in which we display the filling
 */
void flood_fill(
        cv::Mat& edgemap_8uc1_img,
        cv::Mat& heightmap_show_8uc3_img,
        const int x,
        const int y
    ) {
    cv::Mat tmp_edgemap_8uc1_img;

} //flood_fill


/**
 * Find the minimum and maximum coordinates in the file.
 * Note that the file is the S-JTSK coordinate system.
 */
std::tuple<float, float, float, float, float, float> get_min_max(const std::string& filename) {

    float x, y, z;
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float min_z = std::numeric_limits<float>::max();
    // ::min() returns a number that's almost zero and thus is unusable
    // in a system where all coordinates are negative
    float max_x = std::numeric_limits<float>::max() * -1;
    float max_y = std::numeric_limits<float>::max() * -1;
    float max_z = std::numeric_limits<float>::max() * -1;
    int l_type;

    std::ifstream is { filename, std::ios::binary };

    while (is) {
        is.read((char*)&x, 4);
        is.read((char*)&y, 4);
        is.read((char*)&z, 4);
        is.read((char*)&l_type, 4);

        min_x = std::min(x, min_x);
        min_y = std::min(y, min_y);
        min_z = std::min(z, min_z);
        max_x = std::max(x, max_x);
        max_y = std::max(y, max_y);
        max_z = std::max(z, max_z);
    }

    return { min_x, max_x, min_y, max_y, min_z, max_z };
}


/**
 * Fill the image by data from lidar.
 * All lidar points are stored in a an array that has the dimensions of the image. Then the pixel is assigned
 * a value as an average value range from at the corresponding array element. However, with this simple data access, you will lose data precission.
 * filename - file with binarny data
 * img - input image
 */
void fill_image(
        const std::string& filename,
        cv::Mat& heightmap_8uc1_img,
        float min_x,
        float max_x,
        float min_y,
        float max_y,
        float min_z,
        float max_z
    ) {
    int delta_x, delta_y, delta_z;
    float fx, fy, fz;
    int x, y, l_type;
    int stride;
    int num_points = 1;
    float range = 0.0f;
    
    struct Pixel {
        float sum = 0;
        uint64_t count = 0;

        auto avg() const {
            return sum / count;
        }
    };

    std::vector<std::vector<Pixel>> pixels;
    pixels.reserve(heightmap_8uc1_img.rows);

    for (int i = 0; i < heightmap_8uc1_img.rows; ++i) {
        pixels.emplace_back(heightmap_8uc1_img.cols, Pixel{});
    }

    // zjistime sirku a vysku obrazu
    delta_x = round(max_x - min_x + 0.5f);
    delta_y = round(max_y - min_y + 0.5f);
    delta_z = round(max_z - min_z + 0.5f);

    stride = delta_x;

    std::ifstream is { filename, std::ios::binary };

    while (is) {
        is.read((char*)&fx, 4);
        is.read((char*)&fy, 4);
        is.read((char*)&fz, 4);
        is.read((char*)&l_type, 4);

        const size_t px = (fx - min_x) / delta_x * heightmap_8uc1_img.cols;
        const size_t py = (fy - min_y) / delta_y * heightmap_8uc1_img.rows;

        pixels[py][px].sum += fz;
        ++pixels[py][px].count;
    }

    for (int y = 0; y < heightmap_8uc1_img.rows; ++y) {
        for (int x = 0; x < heightmap_8uc1_img.cols; ++x) {
            const auto avg = pixels[y][x].avg();
            const uchar norm = (avg - min_z) / delta_z * 255;
            heightmap_8uc1_img.at<uchar>(y, x) = norm;
        }
    }

    // 1:
    // We allocate helper arrays, in which we store values from the lidar
    // and the number of these values for each pixel

    // 2:
    // go through the file and assign values to the field
    // beware that in S-JTSK the beginning of the co-ordinate system is at the bottom left,
    // while in the picture it is top left

    // 3:
    // assign values from the helper field into the image

}


void make_edges(const cv::Mat& src_8uc1_img, cv::Mat& edgemap_8uc1_img) {
    cv::Canny(src_8uc1_img, edgemap_8uc1_img, 1, 80);
}


/**
 * Transforms the image so it contains only two values.
 * Threshold may be set experimentally.
 */
void binarize_image(cv::Mat& src_8uc1_img) {
    int x, y;
    uchar value;
}


void dilate_and_erode_edgemap(cv::Mat& edgemap_8uc1_img) {
}


void process_lidar(
        const char* txt_filename,
        const char* bin_filename,
        const char* img_filename
    ) {

    float delta_x, delta_y, delta_z;
    MouseProbe* mouse_probe;

    cv::Mat heightmap_8uc1_img;      // image of source of lidar data
    cv::Mat heightmap_show_8uc3_img; // image to detected areas
    cv::Mat edgemap_8uc1_img;        // image for edges

    const auto [min_x, max_x, min_y, max_y, min_z, max_z] = get_min_max(bin_filename);

    printf("min x: %f, max x: %f\n", min_x, max_x);
    printf("min y: %f, max y: %f\n", min_y, max_y);
    printf("min z: %f, max z: %f\n", min_z, max_z);

    delta_x = max_x - min_x;
    delta_y = max_y - min_y;
    delta_z = max_z - min_z;

    printf("delta x: %f\n", delta_x);
    printf("delta y: %f\n", delta_y);
    printf("delta z: %f\n", delta_z);

    heightmap_8uc1_img = cv::Mat(
            cv::Size { cvRound(delta_x + 0.5f), cvRound(delta_y + 0.5f) },
            CV_8UC1);

    heightmap_show_8uc3_img = cv::Mat(
            cv::Size { cvRound(delta_x + 0.5f), cvRound(delta_y + 0.5f) },
            CV_8UC3);

    edgemap_8uc1_img = cv::Mat(
            cv::Size { cvRound(delta_x + 0.5f), cvRound(delta_y + 0.5f) },
            CV_8UC3);

    create_windows(heightmap_8uc1_img.cols, heightmap_8uc1_img.rows);
    mouse_probe = new MouseProbe(heightmap_8uc1_img, heightmap_show_8uc3_img, edgemap_8uc1_img);

    cv::setMouseCallback(STEP1_WIN_NAME, mouse_probe_handler, mouse_probe);
    cv::setMouseCallback(STEP2_WIN_NAME, mouse_probe_handler, mouse_probe);

    printf("Image w=%d, h=%d\n", heightmap_8uc1_img.cols, heightmap_8uc1_img.rows);

    // fill the image with data from lidar scanning
    fill_image(bin_filename, heightmap_8uc1_img, min_x, max_x, min_y, max_y, min_z, max_z);
    cv::cvtColor(heightmap_8uc1_img, heightmap_show_8uc3_img, cv::COLOR_GRAY2RGB);

    // create edge map from the height image
    //make_edges(heightmap_8uc1_img, edgemap_8uc1_img);

    // binarize image, so we can easily process it in the next step
    //binarize_image(edgemap_8uc1_img);
    
    // implement image dilatation and erosion
    //dilate_and_erode_edgemap(edgemap_8uc1_img);

    cv::imwrite(img_filename, heightmap_8uc1_img);

    // wait here for user input using (mouse clicking)
    while (1) {
        cv::imshow(STEP1_WIN_NAME, heightmap_show_8uc3_img);
        //cv::imshow(STEP2_WIN_NAME, edgemap_8uc1_img);
        int key = cv::waitKey(10);
        if (key == 'q') {
            break;
        }
    }
}


int main(int argc, const char* argv[]) {
    if (argc < 4) {
        puts("Not enough command line parameters.");
        exit(1);
    }

    const auto txt_file = argv[1];
    const auto bin_file = argv[2];
    const auto img_file = argv[3];

    process_lidar(txt_file, bin_file, img_file);
}
