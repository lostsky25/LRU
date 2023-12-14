/*
g++ -I/usr/local/include main.cpp /usr/local/lib/libmupdf.a /usr/local/lib/libmupdf-third.a \
-lm -lpng -lsfml-graphics -lsfml-window -lsfml-system && ./a.out py.pdf
*/

#include <iostream>
#include <unordered_map>
#include <list>

#include <SFML/Graphics.hpp>
#include <stdio.h>
#include <stdlib.h>

#include <mupdf/fitz.h>

#include "cache/cache.hpp"

struct Size {
	Size(int _width, int _height) : width(_width), height(_height) {}

	int width;
	int height;
};

class Page {
public:
	Page() = default;

	Page(int _page_id, Size _size) : size(_size), page_id(_page_id) {}

	int getPage() const {
		return page_id;
	}

	Size getSize() const {
		return size;
	}

	std::vector<sf::Uint8> getData() {
		return data;
	}

	void setData(std::vector<sf::Uint8>& data) {
		this->data = data;
	}

	int getId() const {
		return page_id;
	}

private:
	int page_id = 0;
	Size size;
	std::vector<sf::Uint8> data;
};

class PDFFile {
public:
	PDFFile(std::string fileName) {
		/* Create a context to hold the exception stack and various caches. */
		ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
		if (!ctx)
		{
			// fprintf(stderr, "cannot create mupdf context\n");
			// return EXIT_FAILURE;
		}

		/* Register the default file types to handle. */
		fz_try(ctx)
			fz_register_document_handlers(ctx);
		fz_catch(ctx)
		{
			// fprintf(stderr, "cannot register document handlers: %s\n", fz_caught_message(ctx));
			fz_drop_context(ctx);
			// return EXIT_FAILURE;
		}

		/* Open the document. */
		fz_try(ctx)
			doc = fz_open_document(ctx, fileName.c_str());
		fz_catch(ctx)
		{
			// fprintf(stderr, "cannot open document: %s\n", fz_caught_message(ctx));
			fz_drop_context(ctx);
			// return EXIT_FAILURE;
		}

		/* Count the number of pages. */
		fz_try(ctx)
			page_count = fz_count_pages(ctx, doc);
		fz_catch(ctx)
		{
			// fprintf(stderr, "cannot count number of pages: %s\n", fz_caught_message(ctx));
			fz_drop_document(ctx, doc);
			fz_drop_context(ctx);
			// return EXIT_FAILURE;
		}

		if (page_number < 0 || page_number >= page_count)
		{
			// fprintf(stderr, "page number out of range: %d (page count %d)\n", page_number + 1, page_count);
			fz_drop_document(ctx, doc);
			fz_drop_context(ctx);
			// return EXIT_FAILURE;
		}
	}

	Page getPage(int page_number) {
		int zoom = 200, rotate = 0;
		fz_page *fz_page = fz_load_page(ctx, doc, 1 - 1);
		fz_matrix transform = fz_scale(zoom / 100.0f, zoom / 100.0f);
		transform = fz_concat(transform, fz_rotate(rotation));
		/* Compute a transformation matrix for the zoom and rotation desired. */
		/* The default resolution without scaling is 72 dpi. */
		ctm = fz_scale(zoom / 100, zoom / 100);
		ctm = fz_pre_rotate(ctm, rotate);
			/* Render page to an RGB pixmap. */
		fz_try(ctx)
			pix = fz_new_pixmap_from_page_number(ctx, doc, page_number, ctm, fz_device_rgb(ctx), 0);
		fz_catch(ctx)
		{
			// fprintf(stderr, "cannot render page: %s\n", fz_caught_message(ctx));
			fz_drop_document(ctx, doc);
			fz_drop_context(ctx);
			// return EXIT_FAILURE;
		}

		Page page(page_number, Size(pix->w, pix->h));
		// for (y = 0; y < pix->h; ++y)
		// {
		// 	unsigned char *p = &pix->samples[y * pix->stride];
		// 	for (x = 0; x < pix->w; ++x)
		// 	{
		// 		if (x > 0)
		// 			printf("  ");
		// 		printf("%3d %3d %3d", p[0], p[1], p[2]);
		// 		p += pix->n;
		// 	}
		// 	printf("\n");
		// }
		std::vector<sf::Uint8> v(pix->w * pix->h * 4, 0);

		for(int y = 0; y < pix->h; y++)
		{
			unsigned char *p = &pix->samples[y * pix->stride];
			for(int x = 0; x < pix->w; x++)
			{
				// printf("%3d %3d %3d", p[0], p[1], p[2]);
				v[(x + y * pix->w) * 4]     = p[0]; // R?
				v[(x + y * pix->w) * 4 + 1] = p[1]; // G?
				v[(x + y * pix->w) * 4 + 2] = p[2]; // B?
				v[(x + y * pix->w) * 4 + 3] = 255; // A?
				p += pix->n;
			}
		}

		page.setData(v);

		fz_drop_pixmap(ctx, pix);

		return page;
	}

	int getCountPages() const {
		return page_count;
	}

	~PDFFile() {
		/* Clean up. */
		// fz_drop_pixmap(ctx, pix);
		// fz_drop_document(ctx, doc);
		// fz_drop_context(ctx);
	}

private:
	char *input;
	int page_number = 0, page_count;
	fz_context *ctx;
	fz_document *doc;
	fz_pixmap *pix;
	fz_matrix ctm;
	int x, y;
	int rotation = 0;
};

class LRU {
public:
	LRU(PDFFile& pdf, size_t size) : m_size(size){
		for (int id = 0; id < m_size; ++id) {
			Page page = pdf.getPage(id);

			if (pages.size() == m_size) {
				cache.erase(pages.front().getId());
				pages.pop_front();
			}
			pages.push_back(page);
			cache[page.getId()] = &pages.back();
		}
	}

	void d() {
        std::cout << "UNORDERED_MAP (cache)\n" << std::endl;
        for (const auto& el : cache) {
        	std::cout << "\tid: " << el.first << " page_id: " << el.second->getId() << std::endl;
        }
        std::cout << "\nPAGES (list)\n" << std::endl;
        for (const auto& el : pages) {
        	std::cout << "\tid: " << el.getId() << std::endl;
        }
        std::cout << "\033[H\033[2J\033[3J" ;
	}

	void setSize(size_t size) {
		m_size = size;
	}

	size_t getSize() const {
		return m_size;
	}

	Page getItem(PDFFile& pdf, int id, int zoom, int rotate) { 
		if (cache.count(id)) {
			return *(cache[id]);
		} else {
			addItem(pdf.getPage(id));
			
			return pages.back();
		}
	}

private:
	void addItem(Page page) {
		if (pages.size() == m_size) {
			cache.erase(pages.front().getId());
			pages.pop_front();
		}
		pages.push_back(page);
		cache[page.getId()] = &pages.back();
	}

	size_t m_size;
	std::list<Page> pages;
	std::unordered_map<size_t, Page*> cache;
};

int main(int argc, char **argv)
{
	std::string fileName;

    if (argc > 1) { 
    	fileName = argv[1];
    }

	sf::RenderWindow window(sf::VideoMode(1000, 1000), "SFML works!");

	PDFFile pdfFile(fileName);
	std::cout << "Count pages: " << pdfFile.getCountPages() << std::endl;

	LRU lru(pdfFile, 10);
	caches::cache_t<Page> cache(10);

	int page_number = 0;

	sf::Texture texture;
	
	Page page = pdfFile.getPage(0);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

			if (event.type == sf::Event::KeyPressed)
			{
			    if (event.key.code == sf::Keyboard::Right)
			    {
					if (page_number < pdfFile.getCountPages()) {
						++page_number;
						if (cache.lookup_update(page_number, std::bind(&PDFFile::getPage, &pdfFile, std::placeholders::_1))) {
							page = (*(cache.m_hash[page_number])).second;
						}
					}
			    }

			    if (event.key.code == sf::Keyboard::Left)
			    {
					if (page_number > 0) {
						--page_number;
						if (cache.lookup_update(page_number, std::bind(&PDFFile::getPage, &pdfFile, std::placeholders::_1))) {
							page = (*(cache.m_hash[page_number])).second;
						}
					}
			    }
			}
        }

        lru.d();

		sf::Image image;
	  	image.create(page.getSize().width, page.getSize().height, page.getData().data());
		texture.create(page.getSize().width, page.getSize().height); 

		sf::Sprite sprite(texture);

	  	texture.loadFromImage(image);
		texture.update(image);

        window.clear();
        window.draw(sprite);
        window.display();
    }

	return EXIT_SUCCESS;
}