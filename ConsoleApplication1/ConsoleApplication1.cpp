//Uwaga! Co najmniej C++17!!!
//Project-> ... Properties->Configuration Properties->General->C++ Language Standard = ISO C++ 17 Standard (/std:c++17)

#include "SFML/Graphics.hpp"
#include <fstream>

enum class Field { VOID, FLOOR, WALL, BOX, PARK, PLAYER };

class Sokoban : public sf::Drawable
{
public:
	Sokoban()
	{
		Texture_set();
	}
	void LoadMapFromFile(std::string fileName);
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
	void SetDrawParameters(sf::Vector2u draw_area_size);
	void Move_Player_Left();
	void Move_Player_Right();
	void Move_Player_Up();
	void Move_Player_Down();
	bool Is_Victory() const;

	void Texture_set();

private:
	std::vector<std::vector<Field>> map;
	sf::Vector2f shift, tile_size;
	sf::Vector2i player_position;
	std::vector<sf::Vector2i> park_positions;

	std::pair<std::vector<std::vector<Field>>, sf::Vector2i> lastMove; // Ostatni ruch
	bool lastMoveSaved = false; // Czy ostatni ruch został zapisany
	sf::Texture texture_wall;
	sf::Texture texture_background;
	sf::Texture texture_queen;
	sf::Texture texture_spot;
	sf::Texture texture_knight;

	void move_player(int dx, int dy);
};

void Sokoban::LoadMapFromFile(std::string fileName)
{
	std::string str;
	std::vector<std::string> vos;

	std::ifstream in(fileName.c_str());
	while (std::getline(in, str)) { vos.push_back(str); }
	in.close();

	map.clear();
	map.resize(vos.size(), std::vector<Field>(vos[0].size()));
	for (auto [row, row_end, y] = std::tuple{ vos.cbegin(), vos.cend(), 0 }; row != row_end; ++row, ++y)
		for (auto [element, end, x] = std::tuple{ row->begin(), row->end(), 0 }; element != end; ++element, ++x)
			switch (*element)
			{
			case 'X': map[y][x] = Field::WALL; break;
			case '*': map[y][x] = Field::VOID; break;
			case ' ': map[y][x] = Field::FLOOR; break;
			case 'B': map[y][x] = Field::BOX; break;
			case 'P': map[y][x] = Field::PARK; park_positions.push_back(sf::Vector2i(x, y));  break;
			case 'S': map[y][x] = Field::PLAYER; player_position = sf::Vector2i(x, y);  break;
			}
}

void Sokoban::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	for (int y = 0; y < map.size(); ++y)
	{
		for (int x = 0; x < map.at(y).size(); ++x)
		{
			sf::Sprite picture;

			switch (map[y][x])
			{
			case Field::WALL:
				picture.setTexture(texture_wall);
				break;
			case Field::FLOOR:
				picture.setTexture(texture_background);
				break;
			case Field::BOX:
				picture.setTexture(texture_queen);
				break;
			case Field::PARK:
				picture.setTexture(texture_spot);
				break;
			case Field::PLAYER:
				picture.setTexture(texture_knight);
				break;
			default:
				continue;
			}

			float scale_X = tile_size.x / picture.getTexture()->getSize().x;
			float scale_Y = tile_size.y / picture.getTexture()->getSize().y;

			picture.setScale(scale_X, scale_Y);

			float X = x * tile_size.x + shift.x;
			float Y = y * tile_size.y + shift.y;

			picture.setPosition(X, Y);

			target.draw(picture, states);
		}
	}
}



void Sokoban::SetDrawParameters(sf::Vector2u draw_area_size)
{
	this->tile_size = sf::Vector2f(
		std::min(std::floor((float)draw_area_size.x / (float)map[0].size()), std::floor((float)draw_area_size.y / (float)map.size())),
		std::min(std::floor((float)draw_area_size.x / (float)map[0].size()), std::floor((float)draw_area_size.y / (float)map.size()))
	);
	this->shift = sf::Vector2f(
		((float)draw_area_size.x - this->tile_size.x * map[0].size()) / 2.0f,
		((float)draw_area_size.y - this->tile_size.y * map.size()) / 2.0f
	);
}

void Sokoban::Move_Player_Left()
{
	move_player(-1, 0);
}

void Sokoban::Move_Player_Right()
{
	move_player(1, 0);
}

void Sokoban::Move_Player_Up()
{
	move_player(0, -1);
}

void Sokoban::Move_Player_Down()
{
	move_player(0, 1);
}


void Sokoban::move_player(int dx, int dy)
{
	bool allow_move = false; // Pesymistyczne załóżmy, że gracz nie może się poruszyć.
	sf::Vector2i new_pp(player_position.x + dx, player_position.y + dy); //Potencjalna nowa pozycja gracza.
	Field fts = map[new_pp.y][new_pp.x]; //Element na miejscu na które gracz zamierza przejść.
	Field ftsa = map[new_pp.y + dy][new_pp.x + dx]; //Element na miejscu ZA miejscem na które gracz zamierza przejść. :-D

	//Gracz może się poruszyć jeśli pole na którym ma stanąć to podłoga lub miejsce na skrzynki.
	if (fts == Field::FLOOR || fts == Field::PARK) allow_move = true;
	//Jeśli pole na które chce się poruszyć gracz zawiera skrzynkę to może się on poruszyć jedynie jeśli kolejne pole jest puste lub zawiera miejsce na skrzynkę  - bo wtedy może przepchnąć skrzynkę.
	if (fts == Field::BOX && (ftsa == Field::FLOOR || ftsa == Field::PARK))
	{
		allow_move = true;
		//Przepychamy skrzynkę.
		map[new_pp.y + dy][new_pp.x + dx] = Field::BOX;
		//Oczywiście pole na którym stała skrzynka staje się teraz podłogą.
		map[new_pp.y][new_pp.x] = Field::FLOOR;
	}

	if (allow_move)
	{
		//Przesuwamy gracza.
		map[player_position.y][player_position.x] = Field::FLOOR;
		player_position = new_pp;
		map[player_position.y][player_position.x] = Field::PLAYER;
	}

	//Niestety w czasie ruchu mogły „ucierpieć” miejsca na skrzynkę. ;-(
	for (auto park_position : park_positions) if (map[park_position.y][park_position.x] == Field::FLOOR) map[park_position.y][park_position.x] = Field::PARK;
}


bool Sokoban::Is_Victory() const
{
	//Tym razem dla odmiany optymistycznie zakładamy, że gracz wygrał.
	//No ale jeśli na którymkolwiek miejscu na skrzynki nie ma skrzynki to chyba założenie było zbyt optymistyczne... : -/
	for (auto park_position : park_positions) if (map[park_position.y][park_position.x] != Field::BOX) return false;
	return true;
}

void Sokoban::Texture_set() {
	texture_wall.loadFromFile("dirt.png");
	texture_background.loadFromFile("Inst.png");
	texture_queen.loadFromFile("LightKnight.png");
	texture_spot.loadFromFile("Pic_1.png");
	texture_knight.loadFromFile("DarkQueen.png");
}



int main()
{
	sf::Font font;
	if (!font.loadFromFile("Oswald-VariableFont_wght.ttf"))
	{
		return -1;
	}
	sf::Text text("Victory!", font);
	text.setCharacterSize(100);
	text.setFillColor(sf::Color::Red);

	sf::RenderWindow window(sf::VideoMode(800, 600), "GFK Lab 01", sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
	Sokoban sokoban;

	sokoban.LoadMapFromFile("plansza.txt");
	sokoban.SetDrawParameters(window.getSize());

	while (window.isOpen())
	{
		sf::Event event;

		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::Resized)
			{
				float width = static_cast<float>(event.size.width);
				float height = static_cast<float>(event.size.height);
				window.setView(sf::View(sf::FloatRect(0, 0, width, height)));
				sokoban.SetDrawParameters(window.getSize());
			}
			//Oczywiście tu powinny zostać jakoś obsłużone inne zdarzenia.Na przykład jak gracz naciśnie klawisz w lewo powinno pojawić się wywołanie metody :
			//sokoban.Move_Player_Left();
			//W dowolnym momencie mogą Państwo sprawdzić czy gracz wygrał:
			//sokoban.Is_Victory();


			// Obsługa klawiatury poza pętlą pollEvent
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			{
				sokoban.Move_Player_Left();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			{
				sokoban.Move_Player_Right();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			{
				sokoban.Move_Player_Up();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			{
				sokoban.Move_Player_Down();
			}

		}

		window.clear(); // Czyści okno przed ponownym narysowaniem
		window.draw(sokoban);

		if (sokoban.Is_Victory())
		{
			window.draw(text); // Rysuje tekst zwycięstwa, jeśli gracz wygrał
		}

		window.display();
	}

	return 0;
}