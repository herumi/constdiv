all: main.pdf main-ja.pdf

main-ja.pdf: main-ja.tex main.bib notation.tex
	platex main-ja.tex
	pbibtex main-ja
	ptex2pdf -l -ot -synctex=1 main-ja.tex

main.pdf: main.tex main.bib notation.tex
	platex main.tex
	pbibtex main
	ptex2pdf -l -ot -synctex=1 main.tex
#	ptex2pdf -l --shell-escape -ot -synctex=1 main.tex

conv:
	python3 comma.py

clean:
	rm -rf *.pdf *.aux *.dvi *.log *.bbl *.blg
