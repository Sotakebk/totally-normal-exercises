#include <iostream>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <queue>
#include <vector>

// HACKME !!11 SMH
static const char pos[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
						   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
						   'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
						   'u', 'v', 'w', 'x', 'y', 'z' };

char num_pos(int i)
{
	return pos[i % 36];
}

char pos_num(char c)
{
	for (int i = 0; i < 37; i++)
	{
		if (pos[i] == c)
			return i;
	}
	return -1;
}

/* runtime data */
// map information
static int mapsize_x = 0;
static int mapsize_y = 0;
static bool* mapdata;

#define LOC(x, y) (((y)*mapsize_x) + (x))

int start_x = -1;
int start_y = -1;
int end_x = -1;
int end_y = -1;

enum class Status
{
	NONE,    // have not looked at the point yet
	QUEUED,  // added to queue
	VISITED, // already been in queue
	PATH     // belongs to the path
};

struct cell
{
	int srcx;
	int srcy;
	enum Status status;
};

// pathfinding data for each node
cell* cells;

/* drawing */
// move cursor to x, y
void cursor_xy(int x, int y);
// move cursor to I/O area, please call redraw() to clear the console first
void cursor_out();

// draws the map guide
void draw_guide();
// draws the map itself (walls, floors if apply)
void draw_map(bool fancy);
// draws the Start, End points
void draw_start_end();
// draws the pathfinding data
void draw_path();

// returns the character used for path data at x, y; returns ' ' if status::NONE
char renchar_path(int x, int y);
// returns the character used for walls at x, y
char renchar_wall_fancy(int x, int y);

// clear the console and draw everything again (if applicable); clears I/O zone
void redraw();

/* I/O */
// load map data into
void load_map(const char* filename);
void input_filename();
int input_axis(bool horizontal);
void input_point(bool start);
void input_start_end();
bool input_yn(const char* text);
void input_algorithm();

/* runtime data helper functions */
bool map_iswall(int x, int y);

bool cell_isvalid(int x, int y, Status check);
void cell_set(int x, int y, Status state);

/* pathfinding */
void pf_preparedata();
// called when a valid path is found; walks back from the end to the start, marking path cells as such
int pf_walk(bool animate);

// breadth-first pathfinding; finds the optimal path
int pf_breadth_first(bool animate);

// greedy depth-first pathfinding; doesn't find the optimal path
int pf_greedy_depth_first(bool animate);
bool pf_gdf_step(int x, int y, bool animate);

// A* but kind of bad
int pf_astar(bool animate, float heurweight);

/* finally */
int main();

/* definitions */
void cursor_xy(int x, int y)
{
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (short)x, (short)y });
}

void draw_guide()
{
	cursor_xy(0, 0);
	std::cout << "X";
	for (int i = 0; i < mapsize_y; i++)
	{
		cursor_xy(0, i + 1);
		std::cout << pos[i];
	}
	for (int i = 0; i < mapsize_x; i++)
	{
		cursor_xy(i + 1, 0);
		std::cout << pos[i];
	}
	cursor_xy(0, mapsize_y + 1);
	std::cout << "=======I/O=======";
}

bool map_iswall(int x, int y)
{
	if (x >= 0 && x < mapsize_x && y >= 0 && y < mapsize_y)
		return (mapdata[LOC(x, y)] == true);
	return false;
}

char renchar_wall_fancy(int x, int y)
{
	bool s = map_iswall(x, y);
	if (!s)
		return ' ';

	bool u, d, l, r;
	u = map_iswall(x, y - 1);
	d = map_iswall(x, y + 1);
	l = map_iswall(x - 1, y);
	r = map_iswall(x + 1, y);

	if (u && d && l && r)
		return (char)206; // ╬

	if (u && l && r)
		return (char)202; // ╩
	if (d && l && r)
		return (char)203; // ╦
	if (l && u && d)
		return (char)185; // ╣
	if (r && u && d)
		return (char)204; // ╠

	if (u && l)
		return (char)188; // ╝
	if (u && r)
		return (char)200; // ╚
	if (u && d)
		return (char)186; // ║
	if (d && l)
		return (char)187; // ╗
	if (d && r)
		return (char)201; // ╔
	if (l && r)
		return (char)205; // ═

	return (char)206; // ╬
}

void draw_map(bool fancy)
{
	if (mapdata == nullptr)
		return;
	for (int y = 0; y < mapsize_y; y++)
	{
		cursor_xy(1, y + 1);
		for (int x = 0; x < mapsize_x; x++)
		{
			char c;
			if (fancy)
				c = renchar_wall_fancy(x, y);
			else
			{
				if (mapdata[LOC(x, y)] == false)
					c = ' ';
				else
					c = (char)219;
			}
			std::cout << c;
		}
	}
	cursor_xy(0, mapsize_y + 2);
}

void draw_start_end()
{
	if (start_x != -1 && start_y != -1)
	{
		cursor_xy(start_x + 1, start_y + 1);
		std::cout << 'S';
	}
	if (end_x != -1 && end_y != -1)
	{
		cursor_xy(end_x + 1, end_y + 1);
		std::cout << 'E';
	}
}

char renchar_path(int x, int y)
{
	cell* n = &cells[y * mapsize_x + x];

	if (x == start_x && y == start_y)
		return 'S';
	else if (x == end_x && y == end_y)
		return 'E';

	if (n->status == Status::NONE)
		return ' ';
	else if (n->status == Status::QUEUED)
		return '?';
	else if (n->status == Status::VISITED)
		return '.';

	bool l, r, u, d;
	// check if a neighbour cell is a path AND this cell is its source
	l = cell_isvalid(x - 1, y, Status::PATH) && cells[LOC(x - 1, y)].srcx == 1;
	r = cell_isvalid(x + 1, y, Status::PATH) && cells[LOC(x + 1, y)].srcx == -1;
	u = cell_isvalid(x, y - 1, Status::PATH) && cells[LOC(x, y - 1)].srcy == 1;
	d = cell_isvalid(x, y + 1, Status::PATH) && cells[LOC(x, y + 1)].srcy == -1;

	// otherwise, check where is this cells source
	l = (l || cells[LOC(x, y)].srcx == -1);
	r = (r || cells[LOC(x, y)].srcx == 1);
	u = (u || cells[LOC(x, y)].srcy == -1);
	d = (d || cells[LOC(x, y)].srcy == 1);

	if (u)
	{
		if (l)
			return (char)217; // ┘
		else if (r)
			return (char)192; // └
		else
			return (char)179; // │
	}

	if (d)
	{
		if (l)
			return (char)191; // ┐
		else if (r)
			return (char)218; // ┌
	}

	return (char)196; // ─
}

void draw_path()
{
	if (cells == nullptr)
		return;

	for (int y = 0; y < mapsize_y; y++)
	{
		for (int x = 0; x < mapsize_x; x++)
		{
			char c = renchar_path(x, y);
			if (c == ' ')
				continue;
			else
			{
				cursor_xy(x + 1, y + 1);
				std::cout << c;
			}
		}
	}
	cursor_xy(0, mapsize_y + 2);
}

void cursor_out()
{
	std::cout << std::flush;
	cursor_xy(0, mapsize_y + 2);
}

void redraw()
{
	std::cout << std::flush;
	system("CLS");

	draw_guide();
	draw_map(true);
	draw_start_end();
	draw_path();

	cursor_out();
	std::cout << std::flush;
}

void load_map(const char* filename)
{
	std::fstream file(filename, std::fstream::in);
	file >> mapsize_x;
	file >> mapsize_y;

	if (mapdata != nullptr)
	{
		delete[] mapdata;
		mapdata = nullptr;
	}
	mapdata = new bool[mapsize_x * mapsize_y];

	for (int y = 0; y < mapsize_y; y++)
	{
		for (int x = 0; x < mapsize_x; x++)
		{
			char c;
			file >> c;
			if (c == '1')
				mapdata[LOC(x, y)] = 1;
			else
				mapdata[LOC(x, y)] = 0;
		}
	}

	file.close();
}

bool cell_isvalid(int x, int y, Status check)
{
	return (x >= 0 && x < mapsize_x&& y >= 0 && y < mapsize_y) // bounds test
		&& (mapdata[LOC(x, y)] != 1)                         // wall test
		&& (cells[LOC(x, y)].status == check);               // status check
}

void cell_set(int x, int y, Status state)
{
	cells[LOC(x, y)].status = state;
}

void pf_preparedata()
{
	if (cells != nullptr)
	{
		delete[] cells;
		cells = nullptr;
	}

	cells = new cell[mapsize_x * mapsize_y];

	for (int i = 0; i < mapsize_x * mapsize_y; i++)
	{
		cells[i].srcx = 0;
		cells[i].srcy = 0;
		cells[i].status = Status::NONE;
	}
}

int pf_walk(bool animate)
{
	cell_set(start_x, start_y, Status::PATH);

	int x = end_x;
	int y = end_y;  // node position in 2D
	int length = 1; // counted nodes
	// walk backwards
	while (!(x == start_x && y == start_y))
	{
		length++;
		cell_set(x, y, Status::PATH);
		cell* n = &cells[LOC(x, y)];
		if (animate)
		{
			cursor_xy(x + 1, y + 1);
			std::cout << '#';
			Sleep(30);
		}
		x += n->srcx;
		y += n->srcy;
	}

	return length;
}

int pf_breadth_first(bool animate) // <3
{
	struct point
	{
		int x;
		int y;
	};

	std::queue<point> open;

	pf_preparedata();

	open.push({ start_x, start_y });

	bool found = false;
	while (!open.empty())
	{
		point p = open.front(); // active element
		open.pop();
		cells[p.y * mapsize_x + p.x].status = Status::VISITED;

		if (p.x == end_x && p.y == end_y)
		{
			found = true;
			break;
		}

		static const int xdif[] = { 1, 0, -1, 0 };
		static const int ydif[] = { 0, 1, 0, -1 };
		for (int i = 0; i < 4; i++)
		{
			point o = { p.x + xdif[i], p.y + ydif[i] }; // neighbour
			if (cell_isvalid(o.x, o.y, Status::NONE))
			{
				cell* ngbr = &cells[LOC(o.x, o.y)];
				ngbr->status = Status::QUEUED;
				ngbr->srcx = -xdif[i];
				ngbr->srcy = -ydif[i];
				open.push(o);

				if (animate)
				{
					cursor_xy(o.x + 1, o.y + 1);
					std::cout << renchar_path(o.x, o.y);
					Sleep(10);
				}
			}
		}
		if (animate)
		{
			cursor_xy(p.x + 1, p.y + 1);
			std::cout << renchar_path(p.x, p.y);
			Sleep(20);
		}
	}

	// empty the queue
	std::queue<point>().swap(open);

	if (found) // epic success!
		return pf_walk(animate);

	return -1;
}

int pf_astar(bool animate, float heurweight)
{
	struct point
	{
		int x = 0;
		int y = 0;

		int s = 0;   // steps already taken from the start
		float h = 0; // total cell weight

		point(int _x, int _y, int _s, int endx, int endy, float dweight)
		{
			x = _x;
			y = _y;
			s = _s;
			int d = (abs(x - endx) + abs(y - endy));
			h = s + (d * dweight);
		}

		bool operator<(const point& rhs) const
		{
			return h > rhs.h; // we want to look through the cells with the smallest values first
		}
	};

	std::priority_queue<point> open;

	pf_preparedata();

	open.push(point(start_x, start_y, 0, end_x, end_y, heurweight));

	bool found = false;
	while (!open.empty())
	{
		point p = open.top(); // active element
		open.pop();
		cells[p.y * mapsize_x + p.x].status = Status::VISITED;

		if (p.x == end_x && p.y == end_y)
		{
			found = true;
			break;
		}

		static const int xdif[] = { 1, 0, -1, 0 };
		static const int ydif[] = { 0, 1, 0, -1 };
		for (int i = 0; i < 4; i++)
		{
			point o = point(p.x + xdif[i], p.y + ydif[i], p.s + 1, end_x, end_y, heurweight); // neighbour
			if (cell_isvalid(o.x, o.y, Status::NONE))
			{
				cell* ngbr = &cells[LOC(o.x, o.y)];
				ngbr->status = Status::QUEUED;
				ngbr->srcx = -xdif[i];
				ngbr->srcy = -ydif[i];
				open.push(o);

				if (animate)
				{
					cursor_xy(o.x + 1, o.y + 1);
					std::cout << num_pos(static_cast<int>(o.h));
					Sleep(10);
				}
			}
		}
		if (animate)
		{
			cursor_xy(p.x + 1, p.y + 1);
			std::cout << renchar_path(p.x, p.y);
			Sleep(20);
		}
	}

	// empty the queue
	std::priority_queue<point>().swap(open);

	if (found) // epic success!
		return pf_walk(animate);

	return -1;
}

// a step in recursive greedy depth-first pathfinding algorithm
bool pf_gdf_step(int x, int y, bool animate)
{
	cells[LOC(x, y)].status = Status::VISITED;

	if (x == end_x && y == end_y)
		return true;

	int dx = (end_x - x);
	int dy = (end_y - y);
	dx = (dx > 0) - (dx < 0);
	dy = (dy > 0) - (dy < 0);

	int _index = 0;
	bool line = (dx == 0 || dy == 0);

	if (dx == 1 && dy != -1)
		_index = 0;
	else if (dy == 1)
		_index = 1;
	else if (dx == -1)
		_index = 2;
	else
		_index = 3;

	static const int itx[] = { 1, 0, -1, 0 };
	static const int ity[] = { 0, 1, 0, -1 };

	static const int iterA[] = { 0, 1, 3, 2 };

	if (animate)
	{
		cursor_xy(x + 1, y + 1);
		std::cout << renchar_path(x, y);
		Sleep(50);
	}

	for (int i = 0; i < 4; i++)
	{
		int it = 0;
		if (line)
			it = (iterA[i] + _index) % 4;
		else
			it = (i + _index) % 4;

		int cx = x + itx[it];
		int cy = y + ity[it];

		cell* n = &cells[LOC(cx, cy)];
		// can we even move there?
		if (cell_isvalid(cx, cy, Status::NONE))
		{
			n->srcx = -itx[it];
			n->srcy = -ity[it];
			if (pf_gdf_step(cx, cy, animate))
			{
				return true;
			}
		}
	}
	return false;
}

int pf_greedy_depth_first(bool animate)
{
	pf_preparedata();
	// if found
	if (pf_gdf_step(start_x, start_y, animate))
		return pf_walk(animate);

	return -1;
}

void input_filename()
{
	cursor_xy(0, mapsize_y + 2);
	while (true)
	{
		std::cout << "File to load? (path or local file name)" << std::endl;
		std::string filename;
		std::cin >> filename;
		std::ifstream f(filename.c_str());

		if (f.good())
		{
			std::cout << "File " << filename << " found, loading..." << std::endl;
			load_map(filename.c_str());
			break;
		}
		else
			std::cout << "File " << filename << " does not exist!" << std::endl;
	}
}

int input_axis(bool horizontal)
{
	while (true)
	{
		int max = 0;
		if (horizontal)
		{
			std::cout << "X (horizontal): ";
			max = mapsize_x;
		}
		else
		{
			std::cout << "Y (vertical): ";
			max = mapsize_y;
		}

		char pos;
		std::cin >> pos;
		int value = pos_num(pos);

		if (value >= max || value < 0)
			std::cout << "Value out of bounds! must be in range <0, " << num_pos(max - 1) << ">, got: " << num_pos(value) << std::endl;
		else
			return value;
	}
}

void input_point(bool start)
{
	// input a valid point
	while (true)
	{
		std::cout << "Please define the " << (start ? "start" : "end") << " point (example input: 5 a)" << std::endl;
		int valx = input_axis(true);
		int valy = input_axis(false);

		if (map_iswall(valx, valy)) // is in wall
		{
			std::cout << "Point (" << num_pos(valx) << ", " << num_pos(valy) << ") is in a wall, please pick a different position." << std::endl;
		}
		else
		{
			if (start)
			{
				start_x = valx;
				start_y = valy;
			}
			else
			{
				end_x = valx;
				end_y = valy;
			}
			break;
		}
	}
}

void input_start_end()
{
	while (true)
	{
		input_point(true);
		redraw();
		input_point(false);
		redraw();

		if (input_yn("Start and end positions displayed, is this right? y/n"))
			return;

		start_x = -1;
		end_x = -1;
	}
}

bool input_yn(const char* text)
{
	std::cout << text << std::endl;
	char c;
	std::cin >> c;
	return (c == 'y');
}

void input_algorithm()
{
	static const char* names[] =
	{
		"1. Breadth-first iterative",
		"2. Greedy depth-first resursive",
		"3. A* but bad"
	};
	static const int algcount = 3;

	std::cout << "Which algorithm to use?" << std::endl;
	std::cout << "Available: " << std::endl;
	for (int i = 0; i < algcount; i++)
	{
		std::cout << names[i] << std::endl;
	}

	int n = 0;
	while (true)
	{
		std::cout << "Input a number in the range <1, " << algcount << ">" << std::endl;
		std::cin >> n;
		if (n > 0 && n <= algcount)
		{
			std::cout << "Picked: " << names[(n - 1)] << std::endl;
			break;
		}
		else
		{
			std::cout << "Invalid value: " << n << std::endl;
		}
	}

	bool animate = input_yn("Animate? y/n");

	int length = 0;

	redraw();
	switch (n)
	{
	case (1):
		length = pf_breadth_first(animate);
		break;
	case (2):
		length = pf_greedy_depth_first(animate);
		break;
	case (3):
	{
		std::cout << "Please insert a distance heuristic weight (any floating-point value, for example: 0.5)" << std::endl;
		std::cout << "Example values: 0 - breadth-first; 1 - optimal; >1 - greedy" << std::endl;
		float f;
		std::cin >> f;
		std::cout << "Entered: " << f << std::endl;
		length = pf_astar(animate, f);
		break;
	}
	default:
		return;
	}

	redraw();
	if (length > -1)
		std::cout << "Path found, length: " << length << std::endl;
	else
		std::cout << "No path found..." << std::endl;
	std::cout << std::flush;
	delete[] cells;
	cells = nullptr;
}

int main()
{
	while (true)
	{
		redraw();
		if (mapdata == nullptr || input_yn("Change loaded map? y/n"))
		{
			input_filename();
			redraw();
		}
		input_start_end();

		input_algorithm();

		if (!input_yn("Try again? y/n"))
			break;

		start_x = -1;
		end_x = -1;
	}
	return 0;
}
