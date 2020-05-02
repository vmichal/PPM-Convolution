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

std::array<int, 6> histogram{ {0} };

void add_to_histogram(pixel const p) {
	uint8_t const greyscale = std::round(p.r * 0.2126 + p.g * 0.7152 + p.b * 0.0722);

	histogram[greyscale / 51]++;
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



	auto output_iterator = output_buffer.begin();

	for (int i = 0; i < width; ++i) {
		add_to_histogram(lines[0][i]);
		*output_iterator++ = lines[0][i];
	}

	for (int row = 1; row < height - 1; ++row) {
		add_to_histogram(lines[row][0]);
		*output_iterator++ = lines[row][0];

		pixel const* const current = lines[row], * const previous = lines[row - 1], * const next = lines[row + 1];

		int col = 1;
		int remaining = width - 2;
		for (; remaining; ++col, --remaining) {

			uint8_t const red = clamp(5 * current[col].r - current[col - 1].r - current[col + 1].r - previous[col].r - next[col].r);
			uint8_t const green = clamp(5 * current[col].g - current[col - 1].g - current[col + 1].g - previous[col].g - next[col].g);
			uint8_t const blue = clamp(5 * current[col].b - current[col - 1].b - current[col + 1].b - previous[col].b - next[col].b);

			add_to_histogram({ red, green, blue });
			*output_iterator++ = { red, green, blue };
		}

		add_to_histogram(lines[row][width - 1]);
		*output_iterator++ = lines[row][width - 1];
	}

	for (int i = 0; i < width; ++i) {
		add_to_histogram(lines[height - 1][i]);
		*output_iterator++ = lines[height - 1][i];
	}

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