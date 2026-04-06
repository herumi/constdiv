all: main-en.pdf main.pdf

main-en.pdf: main-en.tex main.bib notation.tex
	platex main-en.tex
	pbibtex main-en
	ptex2pdf -l -ot -synctex=1 main-en.tex

main.pdf: main.tex main.bib notation.tex
	platex main.tex
	pbibtex main
	ptex2pdf -l -ot -synctex=1 main.tex
#	ptex2pdf -l --shell-escape -ot -synctex=1 main.tex

conv:
	python3 comma.py

clean:
	rm -rf main.pdf *.aux *.dvi *.log main.bbl main.blg
