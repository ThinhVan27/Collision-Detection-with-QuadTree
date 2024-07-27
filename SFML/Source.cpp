#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Audio.hpp>
#include <chrono>
#include <cmath>
#include <random>
#include <vector>
using namespace std;

class Point;
class Rect;
class QuadTree;

mt19937_64 rd(chrono::steady_clock::now().time_since_epoch().count());

const float FPS = 60;
const int w = 1500;
const int h = 1500;

static long long Rand(long long l, long long r) {
	return l + rd() % (r - l + 1);
}
sf::RenderWindow window(sf::VideoMode(w, h), "My window");
//videomode(width, height)

class Point {
public:
	int id;
	float R, mass;
	sf::Vector2f position;
	sf::Vector2f velocity;
	sf::Color color;

	Point(float R, float x, float y,int id = 0) {
		this->R = R * 1.f;
		position = { x * 1.f,y * 1.f };
		velocity = {
			((Rand(1,243) % 2) ? -1 : 1) * Rand(100,200) * 1.f,
			((Rand(1,10) % 2) ? -1 : 1) * Rand(100,200) * 1.f };
		mass = R * R * R;
		color = sf::Color::Color(Rand(0, 255), Rand(0, 255), Rand(0, 255));
		this->id = id;
	}
	void draww() const {
		sf::CircleShape circle;
		circle.setRadius(R);
		circle.setPosition({ position.x - R,position.y - R });
		circle.setFillColor(color);
		window.draw(circle);

	}
};


class Rect {
public:
	float x, y, w, h;
	Rect(float x = 0, float y= 0, float w = ::w, float h=::h) : x(x), y(y), w(w), h(h) {};

	bool isContain(Point point) const {
		float X = point.position.x;
		float Y = point.position.y;
		return (X >= this->x && X < this->x + this->w && Y >=this->y && Y < this->y + this->h);
	}

	bool intersect(Rect range) const {
		return (range.x < this->x + this->w  && range.x >= this->x) || (range.y  >= this->y && range.y < this->y + this->h);
	}

	~Rect() {};
};

class QuadTree {
private:
	Rect boundary;
	int capacity;
	vector<Point> entities;
	bool divided = false;
	QuadTree* northwest = nullptr, * northeast = nullptr, * southwest = nullptr, * southeast = nullptr;
	//int level;

public:
	QuadTree(Rect boundary, int capacity) {
		this->boundary = boundary;
		this->capacity = capacity;
	}

	~QuadTree() {
		if (northwest) delete northwest;
		if (northeast) delete northeast;
		if (southwest) delete southwest;
		if (southeast) delete southeast;
		entities.clear();
	}

	bool insert(Point point) {
		if (!this->boundary.isContain(point)) return false;
		if (this->entities.size() < this->capacity) {
			entities.push_back(point);
			this->capacity++;
			return true;
		}
		else {
			if (!this->divided) this->subdivided();

			if (this->northwest->insert(point)) {
				return true;
			}
			else if (this->northeast->insert(point)) {
				return true;
			}
			else if (this->southwest->insert(point)) {
				return true;
			}
			else if (this->southeast->insert(point)) {
				return true;
			}
		}
		return true;
	}

	void subdivided() {
		float x = this->boundary.x;
		float y = this->boundary.y;
		float w = this->boundary.w;
		float h = this->boundary.h;
		this->northwest = new QuadTree(Rect(x,y,w/2,h/2), this->capacity);
		this->northeast = new QuadTree(Rect(x+w/2, y, w / 2, h / 2), this->capacity);
		this->southwest = new QuadTree(Rect(x, y+h/2, w / 2, h / 2), this->capacity);
		this->southeast = new QuadTree(Rect(x+w/2, y+h/2, w / 2, h / 2), this->capacity);
		this->divided = true;
	};

	void query(Rect range, vector<Point>& found) {
		found.clear();
		if (!this->boundary.intersect(range)) {
			return;
		}
		else {
			for (Point& p : this->entities) {
				if (range.isContain(p)) {
					found.push_back(p);
				}
			}
			if (this->divided) {
				this->northwest->query(range, found);
				this->northeast->query(range, found);
				this->southwest->query(range, found);
				this->southeast->query(range, found);
			}
		}
		return;
	}

};

static float getDistance(Point a, Point b) {
	return sqrt((a.position.x - b.position.x) * (a.position.x - b.position.x) + (a.position.y - b.position.y) * (a.position.y - b.position.y));
}

static bool isCollision(Point a, Point b) {
	if (getDistance(a,b)
		<= a.R + b.R)
		return true;
	return false;
}


static void update(Point& a, Point& b) {
	if (getDistance(a, b) < a.R + b.R) {
		sf::Vector2f u = { b.position.x - a.position.x,b.position.y - a.position.y };
		u /= sqrt(u.x * u.x + u.y * u.y);
		float len = (a.R + b.R - getDistance(a, b))/2.f;
		u *= len;
		a.position -= u;
		b.position += u;
	}
	sf::Vector2f un = { a.position.x - b.position.x, a.position.y - b.position.y };
	float len = sqrt((un.x) * (un.x) + (un.y) * (un.y));
	un /= len;

	sf::Vector2f ut = { -1.f*(un.y), un.x };

	float v1t = a.velocity.x * ut.x + a.velocity.y * ut.y;
	float v2t = b.velocity.x * ut.x + b.velocity.y * ut.y;
	float v1n = a.velocity.x * un.x + a.velocity.y * un.y;
	float v2n = b.velocity.x * un.x + b.velocity.y * un.y;

	float new_v1n = (v1n * (a.mass - b.mass) + 2 * b.mass * v2n) / (a.mass + b.mass);
	float new_v2n = (v2n * (b.mass - a.mass) + 2 * a.mass * v1n) / (a.mass + b.mass);

	a.velocity = v1t * ut + new_v1n * un;
	b.velocity = v2t * ut + new_v2n * un;

}

static void update(Point& a) {
	sf::Vector2f& pos = a.position;
	pos.x += a.velocity.x * (1.0f / FPS);
	pos.y += a.velocity.y * (1.0f / FPS);

	if (pos.x + a.R >w) {
		a.velocity.x *= -1.f;
		pos.x = w - a.R;
	}
	if (pos.x - a.R < 0) {
		a.velocity.x *= -1.f;
		pos.x = a.R;
	}
	if (pos.y + a.R > h) {
		a.velocity.y *= -1.f;
		pos.y = h - a.R;
	}
	if (pos.y - a.R < 0) {
		a.velocity.y *= -1.f;
		pos.y = a.R;
	}
}

void draw_rec(Rect a)
{
	sf::RectangleShape rectangle;
	rectangle.setSize(sf::Vector2f(a.w , a.h));
	rectangle.setFillColor(sf::Color::Transparent);
	rectangle.setOutlineColor(sf::Color{ 255, 0,0 });
	rectangle.setOutlineThickness(2);
	rectangle.setPosition(a.x, a.y);
	window.draw(rectangle);
}


int main() {
	sf::Mouse mouse();
	window.setFramerateLimit(FPS);
	vector<Point> points;
	Rect a(0.f, 0.f, 1500.f, 1500.f);
	Point circle1(15, 700, 700,0);
	Point circle2(10, 400, 500,1);
	Point circle3(5, 100, 200,2);
	int k = 4;
	points.push_back(circle1);
	points.push_back(circle2);
	points.push_back(circle3);
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
				sf::Vector2i mouse = sf::Mouse::getPosition(window);
					Point circle(Rand(10,30), mouse.x*1.f, mouse.y*1.f,k++);
					points.push_back(circle);
				
			}
		}

		window.clear(sf::Color::Black);


		QuadTree qt(a, 4);
		if (points.size())
			for (Point& i : points)
			{
				qt.insert(i);
			}
		if (points.size())
			for (Point& i : points) {
				vector <Point> res;
				qt.query(Rect( i.position.x-60,i.position.y-60,120 ,120 ), res);
				for (Point& j : res)
				{
					if (i.id != j.id && isCollision(i,j)) {
						update(i, j);
					}
				}
			}

		for (Point& i : points) {
			update(i);
			i.draww();
		}

		window.display();
	}
	return 0;
}