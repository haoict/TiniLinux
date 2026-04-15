.PHONY: board

board:
	@echo "Choose a board (or input a custom name):"
	@echo "  1) h700"
	@echo "  2) rgb30"
	@echo "  3) pi3b"
	@echo "  4) pi3b_development"
	@while true; do \
		printf "Your choice: "; \
		read choice; \
		case "$$choice" in \
			1) BOARD=h700; break;; \
			2) BOARD=rgb30; break;; \
			3) BOARD=pi3b; break;; \
			4) BOARD=pi3b_development; break;; \
			*) BOARD=$$choice; break;; \
		esac; \
	done; \
	echo ""; \
	./scripts/make-board-build.sh configs/$${BOARD}_defconfig
