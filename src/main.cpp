#include <SFML/Graphics.hpp>

#include <array>
#include <string>
#include <stdint.h>
using u8 = uint8_t;
using u32 = uint32_t;
using i32 = int32_t;
using f32 = float;


class Board
{
public:
	Board(const std::filesystem::path& textureFilePath) : debugFont("Assets/LouisGeorgeCafe.ttf") , debugText(debugFont, "EMPTY", 10)
	{
//
//		if (!std::filesystem::exists(textureFilePath))
//		{
//			std::cerr << "File does not exist in: " << std::filesystem::absolute(textureFilePath) << '\n';
//			return;
//		}
		texture_ = std::make_unique<sf::Texture>(textureFilePath);
		sprite_ = std::make_unique<sf::Sprite>(*texture_);
	}

	void Draw(sf::RenderWindow* const window) const
	{
		window->draw(*sprite_);

		for (i32 row = GRID_HEIGHT - 1; row >= 0; --row)
		{
			for (i32 column = 0; column < GRID_WIDTH; column++)
			{
				if (grids_[row][column] == EBoardSlotStatus::Empty)
				{
					debugText.setPosition({ column * 64.f, row * 64.f + 64.f });
					window->draw(debugText);
				}
			}
		}
	}

	enum class EBoardSlotStatus : u8
	{
		Empty,
		Filled_P1,
		Filled_P2
	};

	static constexpr u8 GRID_WIDTH = 7;
	static constexpr u8 GRID_HEIGHT = 6;
private:
	std::unique_ptr<sf::Texture> texture_;
	std::unique_ptr<sf::Sprite> sprite_;
	std::array<std::array<EBoardSlotStatus, GRID_WIDTH>, GRID_HEIGHT> grids_{}; // [Row][Column]

	// TEMP:
	sf::Font debugFont;
	mutable sf::Text debugText;
public:
	inline sf::Sprite* const GetSprite() const { return sprite_.get(); }
};


enum class EChipColor : u8
{
	Blue,
	Green,
	Pink
};


static void setupBoardSpritePositionAndSize(const Board& board, const sf::RenderWindow& window)
{
	constexpr u32 CHIP_TEXTURE_SIZE = 64;
	const f32 availableHeight = static_cast<f32>(window.getSize().y - CHIP_TEXTURE_SIZE);
	const auto textureSize = board.GetSprite()->getTexture().getSize();
	const f32 scaleX = static_cast<f32>(window.getSize().x) / textureSize.x;
	const f32 scaleY = availableHeight / textureSize.y;
	board.GetSprite()->setScale({ scaleX, scaleY });

	board.GetSprite()->setPosition(
		{0.f,
		static_cast<f32>(CHIP_TEXTURE_SIZE)
		});
}

static i32 GetGridColumn(const sf::Sprite& chip, const f32 boardWidth)
{
	const f32 slotWidth = boardWidth / Board::GRID_WIDTH;
	return static_cast<i32>(chip.getPosition().x / slotWidth);
}

//
// @param chip - The chip sprite to move.
// @param direction - -1 for left or 1 for right. Will be clamped.
//
static void MovePreviewChip(sf::Sprite& chip, i32 direction, const f32 boardWidth)
{
	direction = std::clamp(direction, -1, 1);

	const f32 slotWidth = boardWidth / Board::GRID_WIDTH;
	
	const f32 newX = chip.getPosition().x + slotWidth * direction;
	const f32 firstSlotX = slotWidth / 2.f;
	const f32 lastSlotX = boardWidth - slotWidth / 2.f;
	const f32 clampedX = std::clamp(newX, firstSlotX, lastSlotX);
	chip.setPosition({
		clampedX, chip.getPosition().y
		});
}

static void handlePlayerInput(sf::Keyboard::Scancode scancode, sf::Sprite& chip, const f32 boardWidth)
{
	using SCode = sf::Keyboard::Scancode;
	switch (scancode)
	{
	case SCode::Left:
		MovePreviewChip(chip, -1, boardWidth);
		break;
	case SCode::Right:
		MovePreviewChip(chip, 1, boardWidth);
		break;
	case SCode::Down:
		break;
	}
}


int main()
{
	sf::RenderWindow window( sf::VideoMode( { 600, 550 } ), "Connect4",  sf::Style::Titlebar | sf::Style::Close);
	// window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);
	window.setKeyRepeatEnabled(false);
	
	// MY STUFF.

	const sf::Texture chipGreenTex("Assets/chipGreen.png");
	const sf::Texture chipPinkTex("Assets/chipPink.png");

	using Turn = EChipColor;

	Turn playerInTurn = Turn::Green;
	sf::Sprite currentChip(chipGreenTex);// By default green will be player 1.
	currentChip.setOrigin({ chipGreenTex.getSize().x / 2.f, chipGreenTex.getSize().y / 2.f });
	currentChip.setPosition({ static_cast<f32>(window.getSize().x) / 2, chipGreenTex.getSize().y / 2.f});

	constexpr const char* BOARD_TEXTURE_PATH = "Assets/Board.png";
	Board board(BOARD_TEXTURE_PATH);
	
	setupBoardSpritePositionAndSize(board, window);
	
	
	while ( window.isOpen() )
	{
		while ( const std::optional event = window.pollEvent() )
		{
			if ( event->is<sf::Event::Closed>() )
				window.close();
			
			if (event->is<sf::Event::KeyPressed>())
			{
				// Close with ESC key.
				const auto* key = event->getIf<sf::Event::KeyPressed>();
				if (key->scancode == sf::Keyboard::Scancode::Escape)
				{
					window.close();
				}
				handlePlayerInput(key->scancode, currentChip, static_cast<f32>(window.getSize().x));
				
			}
			// if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			//if (keyPressed->code == sf::Keyboard::Key::Up)
			//	LOG("Pressed up")
		}

		// Rendering stage.
		window.clear();
		
		board.Draw(&window);
		window.draw(currentChip);
		
		window.display();
	}
}
