# coding: utf-8
import os, codecs

def replaceComma(fName):
	if not fName.endswith('tex'):
		return
	print(f'file={fName}')
	fi = codecs.open(fName, 'r', encoding = 'utf-8')
	s = fi.read()
	fi.close()

	s = s.replace(u'、', u'，')
	s = s.replace(u'。', u'．')
	s = s.replace(u'\u00a5', u'\u005c')

	fo = codecs.open(fName, 'w', encoding = 'utf-8')
	fo.write(s)
	fo.close()

def main():
	path = './'
	files = os.listdir(path)
	for f in files:
		if os.path.isfile(os.path.join(path, f)):
			replaceComma(f)

if __name__ == '__main__':
	main()
