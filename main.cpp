//Homework assignment for university course Computer architectures at CTU FEE
//https://cw.fel.cvut.cz/wiki/courses/b35apo/homeworks/02/start

#pragma GCC optimize("Ofast")

#include <iostream>
#include <iterator>
#include <array>
#include <algorithm>
#include <vector>
#include <cassert>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>
#include <cmath>
#include <numeric>

constexpr auto file_magic = "P6";


int print_help(std::string program_name) {
	std::cerr <<
		"Usage: " << program_name << " argument\n"
		"Argument shall be a valid path to ppm picture.\n";

	return 0;
}

bool check_magic(std::ifstream& input) {
	std::string line;
	std::getline(input, line);
	return line == file_magic;
}

struct pixel {
	std::uint8_t r, g, b;
};

static_assert(sizeof(pixel) == 3);

constexpr int clamp(int current) {
	return current > 255 ? 255 : current < 0 ? 0 : current;
}

constexpr int greyscale(pixel const p) {
	return std::round(p.r * 0.2126 + p.g * 0.7152 + p.b * 0.0722);
}


int main(int argc, char** argv) {

	std::ios_base::sync_with_stdio(false);

	if (argc != 2) {
		std::cout << "Incorrect number of arguments.\n";
		return print_help(argv[0]);
	}

	std::ifstream input(argv[1]);

	if (!input.is_open() || !check_magic(input)) {
		std::cerr << "This is not a ppm file!\n";
		return print_help(argv[0]);
	}
	if (input.peek() == '#') {
		std::string comment; //Extract single comment line from input file
		std::getline(input, comment);
	}


	int W, H, max_value;
	input >> W >> H >> max_value;
	assert(W > 0 && H > 0 && max_value == 255);
	input.ignore(1); //ignore the last newline after header
	int const width = W, height = H;

	int const size = width * height;

	std::vector<pixel> pixels(size);
	std::vector<pixel> output_buffer(size);
	input.read(reinterpret_cast<char*>(pixels.data()), size * sizeof(pixel));

	std::vector<pixel const*> lines(height);
	for (int i = 0; i < height; ++i)
		lines[i] = pixels.data() + width * i;


	std::array<int, 6> histogram{ {0} };

	auto output_iterator = output_buffer.begin();
	auto const finalize_pixel = [&](pixel const pixel) {
		uint8_t const grey = greyscale(pixel);

		histogram[grey / 51]++;
		*output_iterator = pixel;
		++output_iterator;
	};

	for (int i = 0; i < width; ++i)
		finalize_pixel(lines[0][i]);

	for (int row = 1; row < height - 1; ++row) {
		finalize_pixel(lines[row][0]);

		pixel const* const current = lines[row], * const previous = lines[row - 1], * const next = lines[row + 1];

		for (int col = 1; col < width - 1; ++col) {

			uint8_t const red = clamp(5 * current[col].r - current[col - 1].r - current[col + 1].r - previous[col].r - next[col].r);
			uint8_t const green = clamp(5 * current[col].g - current[col - 1].g - current[col + 1].g - previous[col].g - next[col].g);
			uint8_t const blue = clamp(5 * current[col].b - current[col - 1].b - current[col + 1].b - previous[col].b - next[col].b);

			finalize_pixel({ red, green, blue });
		}

		finalize_pixel(lines[row][width - 1]);
	}

	for (int i = 0; i < width; ++i)
		finalize_pixel(lines[height - 1][i]);

	histogram[4] += histogram[5]; //accounting for the hacky histogram computation

	{
		std::ostringstream output_header;
		output_header << file_magic << '\n' << width << '\n' << height << '\n' << max_value << '\n';
		std::string const header = output_header.str();

		std::ofstream output("output.ppm", std::ios::binary);
		output << header;
		output.write(reinterpret_cast<char*>(output_buffer.data()), size * sizeof(pixel));
	}

	{
		std::ofstream histogram_output("output.txt");
		std::copy(histogram.begin(), histogram.begin() + 4, std::ostream_iterator<int>(histogram_output, " "));
		histogram_output << histogram[4];

	}
	return 0;

}