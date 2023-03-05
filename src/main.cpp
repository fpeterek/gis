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
    const char* step1_win_name = "Heightmap";
    const char* step2_win_name = "Edges";
    const char* ltype0_win = "First reflection";
    const char* ltype1_win = "Vegetation";
    const char* ltype2_win = "Surface beneath vegetation";
    const int zoom = 1;
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
void flood_fill(const cv::Mat& src_img, cv::Mat& dst_img, int x, int y);


/**
 * Mouse clicking callback.
 */
void mouse_probe_handler(int event, int x, int y, [[maybe_unused]] int flags, void* param) {
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
    cv::namedWindow(step1_win_name, 0);
    cv::namedWindow(step2_win_name, 0);
    cv::namedWindow(ltype0_win, 0);
    cv::namedWindow(ltype1_win, 0);
    cv::namedWindow(ltype2_win, 0);

    cv::resizeWindow(step1_win_name, width*zoom, height*zoom);
    cv::resizeWindow(step2_win_name, width*zoom, height*zoom);
    cv::resizeWindow(ltype0_win, width*zoom, height*zoom);
    cv::resizeWindow(ltype1_win, width*zoom, height*zoom);
    cv::resizeWindow(ltype2_win, width*zoom, height*zoom);

} // create_windows


void fill_step(
        cv::Mat& edgemap,
        cv::Mat& heightmap,
        const int x,
        const int y,
        const int desired_value
    ) {

    if (x < 0 or x >= edgemap.cols or y < 0 or y >= edgemap.rows) {
        return;
    }

    if (edgemap.at<uchar>(y, x) != desired_value) {
        return;
    }
    edgemap.at<uchar>(y, x) = desired_value - 255;
    heightmap.at<cv::Vec3b>(y, x) = { 0, 0, 255 };

    fill_step(edgemap, heightmap, x-1, y, desired_value);
    fill_step(edgemap, heightmap, x+1, y, desired_value);
    fill_step(edgemap, heightmap, x, y-1, desired_value);
    fill_step(edgemap, heightmap, x, y+1, desired_value);
}

void flood_fill(
        const cv::Mat& edgemap_8uc1_img,
        cv::Mat& heightmap_show_8uc3_img,
        const int x,
        const int y
    ) {
    cv::Mat tmp_edge { edgemap_8uc1_img };
    cv::Mat dest { heightmap_show_8uc3_img.clone() };

    fill_step(tmp_edge, dest, x, y, tmp_edge.at<uchar>(y, x));

    heightmap_show_8uc3_img = dest;
}

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
    int l_type;
    
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
}

void fill_image_with_ltype(
        const std::string& filename,
        cv::Mat& dest,
        const float min_x,
        const float max_x,
        const float min_y,
        const float max_y,
        const float min_z,
        const float max_z,
        const int ltype
    ) {
    int delta_x, delta_y, delta_z;
    float fx, fy, fz;
    int l_type;
    
    struct Pixel {
        float sum = 0;
        uint64_t count = 0;

        auto avg() const {
            return sum / count;
        }
    };

    std::vector<std::vector<Pixel>> pixels;
    pixels.reserve(dest.rows);

    for (int i = 0; i < dest.rows; ++i) {
        pixels.emplace_back(dest.cols, Pixel{});
    }

    // zjistime sirku a vysku obrazu
    delta_x = round(max_x - min_x + 0.5f);
    delta_y = round(max_y - min_y + 0.5f);
    delta_z = round(max_z - min_z + 0.5f);

    std::ifstream is { filename, std::ios::binary };

    while (is) {
        is.read((char*)&fx, 4);
        is.read((char*)&fy, 4);
        is.read((char*)&fz, 4);
        is.read((char*)&l_type, 4);

        if (l_type != ltype) {
            continue;
        }

        const size_t px = (fx - min_x) / delta_x * dest.cols;
        const size_t py = (fy - min_y) / delta_y * dest.rows;

        pixels[py][px].sum += fz;
        ++pixels[py][px].count;
    }

    for (int y = 0; y < dest.rows; ++y) {
        for (int x = 0; x < dest.cols; ++x) {
            const auto avg = pixels[y][x].avg();
            const uchar norm = (avg - min_z) / delta_z * 255;
            dest.at<uchar>(y, x) = norm;
        }
    }
}

void make_edges(const cv::Mat& src_8uc1_img, cv::Mat& edgemap_8uc1_img) {
    cv::Canny(src_8uc1_img, edgemap_8uc1_img, 1, 80);
}

/**
 * Transforms the image so it contains only two values.
 * Threshold may be set experimentally.
 */
void binarize_image(cv::Mat& img) {
    for (int y = 0; y < img.rows; ++y) {
        for (int x = 0; x < img.cols; ++x) {
            const auto val = img.at<uchar>(y, x);
            img.at<uchar>(y, x) = 255 * (val > 127);
        }
    }
}

void fill_all(cv::Mat& img, const int x, const int y) {
    for (int dy = -1; dy < 2; ++dy) {
        for (int dx = -1; dx < 2; ++dx) {
            img.at<uchar>(y+dy, x+dx) = 255;
        }
    }
}

cv::Mat dilate(const cv::Mat& img) {
    cv::Mat dest = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);

    for (int y = 1; y < img.rows-1; ++y) {
        for (int x = 1; x < img.cols-1; ++x) {
            const auto val = img.at<uchar>(y, x);

            if (val) {
                fill_all(dest, x, y);
            }
        }
    }

    return dest;
}

void erode_px(const cv::Mat& orig, cv::Mat& dest, const int x, const int y) {
    for (int dy = -1; dy < 2; ++dy) {
        for (int dx = -1; dx < 2; ++dx) {
            if (not orig.at<uchar>(y+dy, x+dx)) {
                return;
            }
        }
    }
    dest.at<uchar>(y, x) = 255;
}

cv::Mat erode(const cv::Mat& img) {
    cv::Mat dest = cv::Mat::zeros(img.rows, img.cols, CV_8UC1);

    for (int y = 1; y < img.rows-1; ++y) {
        for (int x = 1; x < img.cols-1; ++x) {
            erode_px(img, dest, x, y);
        }
    }

    return dest;
}

cv::Mat dilate_and_erode_edgemap(const cv::Mat& img) {
    return erode(dilate(img));
}


void process_lidar(
        [[maybe_unused]] const char* txt_filename,
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
            CV_8UC1);

    create_windows(heightmap_8uc1_img.cols, heightmap_8uc1_img.rows);
    mouse_probe = new MouseProbe(heightmap_8uc1_img, heightmap_show_8uc3_img, edgemap_8uc1_img);

    cv::setMouseCallback(step1_win_name, mouse_probe_handler, mouse_probe);
    cv::setMouseCallback(step2_win_name, mouse_probe_handler, mouse_probe);

    printf("Image w=%d, h=%d\n", heightmap_8uc1_img.cols, heightmap_8uc1_img.rows);

    fill_image(bin_filename, heightmap_8uc1_img, min_x, max_x, min_y, max_y, min_z, max_z);
    cv::cvtColor(heightmap_8uc1_img, heightmap_show_8uc3_img, cv::COLOR_GRAY2RGB);

    make_edges(heightmap_8uc1_img, edgemap_8uc1_img);

    binarize_image(edgemap_8uc1_img);
    
    edgemap_8uc1_img = dilate_and_erode_edgemap(edgemap_8uc1_img);

    cv::imwrite(img_filename, heightmap_8uc1_img);

    cv::Mat fst_reflect {
            cv::Size { cvRound(delta_x + 0.5f), cvRound(delta_y + 0.5f) },
            CV_8UC1 };
    cv::Mat vegetation {
            cv::Size { cvRound(delta_x + 0.5f), cvRound(delta_y + 0.5f) },
            CV_8UC1 };
    cv::Mat beneath_veg {
            cv::Size { cvRound(delta_x + 0.5f), cvRound(delta_y + 0.5f) },
            CV_8UC1 };

    fill_image_with_ltype(
            bin_filename, fst_reflect, min_x, max_x, min_y, max_y, min_z, max_z, 0);
    fill_image_with_ltype(
            bin_filename, vegetation, min_x, max_x, min_y, max_y, min_z, max_z, 1);
    fill_image_with_ltype(
            bin_filename, beneath_veg, min_x, max_x, min_y, max_y, min_z, max_z, 2);

    while (1) {
        cv::imshow(step1_win_name, heightmap_show_8uc3_img);
        cv::imshow(step2_win_name, edgemap_8uc1_img);
        cv::imshow(ltype0_win, fst_reflect);
        cv::imshow(ltype1_win, vegetation);
        cv::imshow(ltype2_win, beneath_veg);
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
