import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

def main():
	window = Gtk.Window(title='Hola Mundo')
	window.show()
	window.connect('destroy', Gtk.main_quit)
	Gtk.main()

if __name__ == '__main__':
	main()
