.PHONY: all
all: thesis

main.bbl:
	xelatex -shell-escape -synctex=1 -interaction=nonstopmode -file-line-error main.tex
	biber main

thesis: main.bbl
	xelatex -shell-escape -synctex=1 -interaction=nonstopmode -file-line-error main.tex

clean:
	git ls-files --ignored --exclude-standard -o | xargs rm
