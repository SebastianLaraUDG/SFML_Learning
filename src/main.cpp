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
	// @param chipPosition - The position of the chip to add. This is used to determine which column to add the chip to.
	// @param colorToInsert - The color of the chip to add.
	// @param outRowIndex - The row index of the chip that was added. This is an output parameter.
	// @param outColumnIndex - The column index of the chip that was added. This is an output parameter.
	// @returns true if the chip was added, false if the column is full.
	bool AddChip(const sf::Vector2f& chipPosition, const EPlayerChip colorToInsert, i32& outRowIndex, i32& outColumnIndex)
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
				outRowIndex = row;
				outColumnIndex = column;
				return true;
			}
		}
		return false;
	}

	// Returns true if the player has won, false otherwise.
	bool CheckVictory(const EPlayerChip& possibleWinner, const i32 rowIndex, const i32 columnIndex) const
	{
		// You'll see all values are initialized to 1, this is because we are assuming that the chip that was just placed
		// is already counted as 1 in the sequence. So we start counting from 1 and then check in all directions for additional chips of the same type.


		EBoardSlotStatus typeToEval = possibleWinner == EPlayerChip::P1 ? EBoardSlotStatus::Filled_P1 : EBoardSlotStatus::Filled_P2;
		
		// Vertical check.
		i32 columnAccum = 1;

		for (i32 row = rowIndex + 1; row < GRID_HEIGHT; ++row)
		{
			if (grids_[row][columnIndex] != typeToEval)
			{
				break;
			}
			columnAccum++;
			if (columnAccum >= 4)
			{
				return true;
			}
		}

		// Check horizontal (left and right).
		i32 rowAccum = 1;
		i32 leftColumn = columnIndex - 1;
		i32 rightColumn = columnIndex + 1;
		// Check left.
		while (leftColumn >= 0 && grids_[rowIndex][leftColumn] == typeToEval)
		{
			leftColumn--;
			rowAccum++;
		}
		// Check right.
		while (rightColumn < GRID_WIDTH && grids_[rowIndex][rightColumn] == typeToEval)
		{
			rightColumn++;
			rowAccum++;
		}
		if (rowAccum >= 4)
		{
			return true;
		}


		// Check diagonal ↖↘ (up-left and down-right).
		i32 diagonalAccum1 = 1;
		i32 upLeftRow = rowIndex - 1;
		i32 upLeftColumn = columnIndex - 1;
		i32 downRightRow = rowIndex + 1;
		i32 downRightColumn = columnIndex + 1;

		// Check up-left.
		while (upLeftRow >= 0 && upLeftColumn >= 0 && grids_[upLeftRow][upLeftColumn] == typeToEval)
		{
			upLeftRow--;
			upLeftColumn--;
			diagonalAccum1++;
		}
		// Check down-right.
		while (downRightRow < GRID_HEIGHT && downRightColumn < GRID_WIDTH && grids_[downRightRow][downRightColumn] == typeToEval)
		{
			downRightRow++;
			downRightColumn++;
			diagonalAccum1++;
		}
		if (diagonalAccum1 >= 4)
		{
			return true;
		}

		// Check diagonal ↗↙ (up-right and down-left).
		i32 diagonalAccum2 = 1;
		i32 upRightRow = rowIndex - 1;
		i32 upRightColumn = columnIndex + 1;
		i32 downLeftRow = rowIndex + 1;
		i32 downLeftColumn = columnIndex - 1;

		// Check up-right.
		while (upRightRow >= 0 && upRightColumn < GRID_WIDTH && grids_[upRightRow][upRightColumn] == typeToEval)
		{
			upRightRow--;
			upRightColumn++;
			diagonalAccum2++;
		}
		// Check down-left.
		while (downLeftRow < GRID_HEIGHT && downLeftColumn >= 0 && grids_[downLeftRow][downLeftColumn] == typeToEval)
		{
			downLeftRow++;
			downLeftColumn--;
			diagonalAccum2++;
		}
		if (diagonalAccum2 >= 4)
		{
			return true;
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

static void handlePlayerInput(sf::Keyboard::Scancode scancode, sf::Sprite& chip, const sf::Texture& greenTexture, const sf::Texture& pinkTexture, const f32 boardWidth, Board& board, EPlayerChip& colorToInsert, bool& bWinner, sf::Text& winnerText)
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
		i32 outRowIndex, outColumnIndex;
		if (board.AddChip(chip.getPosition(), colorToInsert, outRowIndex, outColumnIndex))
		{
			if(board.CheckVictory(colorToInsert, outRowIndex, outColumnIndex))
			{
				bWinner = true;
				winnerText.setString("Player " + std::to_string((colorToInsert == EPlayerChip::P1) ? 1 : 2) + " wins!");
				winnerText.setCharacterSize(48);
				winnerText.setOrigin({ winnerText.getLocalBounds().size.x / 2.f, winnerText.getLocalBounds().size.y / 2.f });
				winnerText.setPosition({ boardWidth / 2.f, static_cast<f32>(winnerText.getCharacterSize()) });
				winnerText.setFillColor(colorToInsert == EPlayerChip::P1 ? sf::Color::Green : sf::Color::Red);
			}
			else
			{
				SwitchPlayerTurn(colorToInsert, chip, greenTexture, pinkTexture);
				chip.setPosition({ boardWidth / 2.f, chip.getPosition().y }); // Reset preview chip to center after placing a chip.})
			}
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

	bool bWinner = false;
	sf::Font winnerFont("Assets/LouisGeorgeCafe.ttf");
	sf::Text winnerText(winnerFont);
	
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
				if(!bWinner)
				handlePlayerInput(key->scancode, currentChip, chipGreenTex, chipPinkTex, boardWidth, board, playerInTurn, bWinner, winnerText);
				
			}
			// if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			//if (keyPressed->code == sf::Keyboard::Key::Up)
			//	LOG("Pressed up")
		}

		// Rendering stage.
		window.clear();
		
		board.Draw(&window, chipGreenTex, chipPinkTex, currentChip);
		window.draw(currentChip);
		if (bWinner)
		{
			window.draw(winnerText);
		}
		
		window.display();
	}
}
