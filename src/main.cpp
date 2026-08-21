#include <SFML/Graphics.hpp>

#include <array>
#include <string>
#include <stdint.h>
using u8 = uint8_t;
using u32 = uint32_t;
using i32 = int32_t;
using f32 = float;

enum class EPlayerChip : u8
{
	P1,
	P2
};


class Board
{
public:
	Board(const std::filesystem::path& textureFilePath)
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

	
	// Add a chip to the bottom of this column, if possible. If the column is full, do nothing.
	// @returns true if the chip was added, false if the column is full.
	bool AddChip(const sf::Vector2f& chipPosition, const EPlayerChip colorToInsert)
	{
		// Convert the chip position to a column index.
		// Then after getting the column index, check if the column is full. If it is not full, find the lowest empty slot in that column and fill it with the chip color.
		// If the column has empty slots, fill the lowest empty slot with the chip color.

		const sf::FloatRect boardBounds = sprite_->getGlobalBounds();
		const f32 slotWidth = boardBounds.size.x / GRID_WIDTH;

		// Restar la posición del tablero para trabajar en espacio local del board.
		const f32 localX = chipPosition.x - boardBounds.position.x;
		const i32 column = static_cast<i32>(localX / slotWidth);

		// Safety check: descartar columnas fuera de rango.
		if (column < 0 || column >= GRID_WIDTH)
		{
			return false;
		}

		// Then after getting the column index, check if the column is full.
		// If it is not full, find the lowest empty slot in that column and fill it with the chip color.
		for (i32 row = GRID_HEIGHT - 1; row >= 0; --row)
		{
			if (grids_[row][column] == EBoardSlotStatus::Empty)
			{
				// If the column has empty slots, fill the lowest empty slot with the chip color.
				grids_[row][column] = (colorToInsert == EPlayerChip::P1) ? EBoardSlotStatus::Filled_P1 : EBoardSlotStatus::Filled_P2;
				return true;
			}
		}
		return false;
	}

	void Draw(sf::RenderWindow* const window, const sf::Texture& p1Texture, const sf::Texture& p2Texture, sf::Sprite& chip) const
	{
		window->draw(*sprite_);

		// Draw the chips in the grid.
		// This is the concept idea:
		// Draw the grid, checking from bottom to top, this is because in connect4 there must be
		// a chip at the bottom of each column for other chips to be placed on top of it. So we will draw the chips from bottom to top, and if a chip is found, we will draw it and then continue to the next row.


		// Guardamos el estado real del preview antes de usar "chip" como herramienta de dibujo.
		const sf::Vector2f originalPosition = chip.getPosition();
		const sf::Texture* const originalTexture = &chip.getTexture();

		const sf::FloatRect boardBounds = sprite_->getGlobalBounds();
		const sf::Vector2f boardPos = boardBounds.position;
		const f32 slotWidth = boardBounds.size.x / GRID_WIDTH;
		const f32 slotHeight = boardBounds.size.y / GRID_HEIGHT;

		for (i32 column = 0; column < GRID_WIDTH; column++)
		{
			for (i32 row = GRID_HEIGHT - 1; row >= 0; --row)
			{
				const EBoardSlotStatus status = grids_[row][column];

				if (status == EBoardSlotStatus::Empty)
				{
					continue;
				}

				const sf::Texture& chipTexture = (status == EBoardSlotStatus::Filled_P1) ? p1Texture : p2Texture;
				chip.setTexture(chipTexture, true);

				const f32 x = boardPos.x + column * slotWidth + slotWidth / 2.f;
				const f32 y = boardPos.y + row * slotHeight + slotHeight / 2.f;

				chip.setPosition({ x, y });
				window->draw(chip);
			}
		}

		// Restauramos el chip a su estado real de preview, como si Draw nunca hubiese existido.
		chip.setTexture(*originalTexture, true);
		chip.setPosition(originalPosition);
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
public:
	inline sf::Sprite* const GetSprite() const { return sprite_.get(); }
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

// Moves the preview chip left or right by one slot, clamping its position within the board's bounds.
// @param chip - The chip sprite to move.
// @param direction - -1 for left or 1 for right. Will be clamped.
// @param boardWidth - The width of the board, used to clamp the chip's position within the board's bounds.
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

static void SwitchPlayerTurn(EPlayerChip& playerInTurn, sf::Sprite& currentChip, const sf::Texture& chipGreenTex, const sf::Texture& chipPinkTex)
{
	if (playerInTurn == EPlayerChip::P1)
	{
		playerInTurn = EPlayerChip::P2;
		currentChip.setTexture(chipPinkTex);
	}
	else
	{
		playerInTurn = EPlayerChip::P1;
		currentChip.setTexture(chipGreenTex);
	}
}

static void handlePlayerInput(sf::Keyboard::Scancode scancode, sf::Sprite& chip, const sf::Texture& greenTexture, const sf::Texture& pinkTexture, const f32 boardWidth, Board& board, EPlayerChip& colorToInsert)
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
		if (board.AddChip(chip.getPosition(), colorToInsert))
		{
			SwitchPlayerTurn(colorToInsert, chip, greenTexture, pinkTexture);
		}
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

	using Turn = EPlayerChip;

	Turn playerInTurn = Turn::P1;
	sf::Sprite currentChip(chipGreenTex);// By default green will be player 1.
	currentChip.setOrigin({ chipGreenTex.getSize().x / 2.f, chipGreenTex.getSize().y / 2.f });
	currentChip.setPosition({ static_cast<f32>(window.getSize().x) / 2, chipGreenTex.getSize().y / 2.f});

	constexpr const char* BOARD_TEXTURE_PATH = "Assets/Board.png";
	Board board(BOARD_TEXTURE_PATH);
	
	setupBoardSpritePositionAndSize(board, window);
	

	const f32 boardWidth = static_cast<f32>(window.getSize().x);
	
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
				handlePlayerInput(key->scancode, currentChip, chipGreenTex, chipPinkTex, boardWidth, board, playerInTurn);
				
			}
			// if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			//if (keyPressed->code == sf::Keyboard::Key::Up)
			//	LOG("Pressed up")
		}

		// Rendering stage.
		window.clear();
		
		board.Draw(&window, chipGreenTex, chipPinkTex, currentChip);
		window.draw(currentChip);
		
		window.display();
	}
}
