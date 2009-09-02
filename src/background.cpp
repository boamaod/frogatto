#include <GL/glew.h>

#include <iostream>
#include <map>

#include "background.hpp"
#include "color_utils.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "level.hpp"
#include "raster.hpp"
#include "thread.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"

namespace {
std::map<std::string, boost::shared_ptr<background> > cache;
threading::mutex cache_mutex;
}

boost::shared_ptr<background> background::get(const std::string& id)
{
	threading::lock lck(cache_mutex);
	boost::shared_ptr<background>& obj = cache[id];
	if(!obj) {
		obj.reset(new background(wml::parse_wml_from_file("data/backgrounds/" + id + ".cfg")));
		obj->id_ = id;
	}

	return obj;
}

std::vector<std::string> background::get_available_backgrounds()
{
	std::vector<std::string> files;
	sys::get_files_in_dir("data/backgrounds/", &files);

	std::vector<std::string> result;
	foreach(const std::string& fname, files) {
		if(fname.size() > 4 && std::equal(fname.end() - 4, fname.end(), ".cfg")) {
			result.push_back(std::string(fname.begin(), fname.end() - 4));
		}
	}

	return result;
}

background::background(const wml::const_node_ptr& node)
{
	top_ = string_to_color(node->attr("top"));
	bot_ = string_to_color(node->attr("bottom"));
	width_ = wml::get_int(node, "width");
	height_ = wml::get_int(node, "height");

	FOREACH_WML_CHILD(layer_node, node, "layer") {
		layer bg;
		bg.image = (*layer_node)["image"];
		bg.image_formula = layer_node->attr("image_formula");
		bg.xscale = wml::get_int(layer_node, "xscale", 100);
		bg.yscale = wml::get_int(layer_node, "yscale", 100);
		bg.xspeed = wml::get_int(layer_node, "xspeed", 0);
		bg.yoffset = wml::get_int(layer_node, "yoffset", 0);
		bg.scale = wml::get_int(layer_node, "scale", 1);
		if(bg.scale < 1) {
			bg.scale = 1;
		}

		std::string blend_mode = layer_node->attr("mode");
		if ( blend_mode == "GL_MAX"){
			bg.mode = GL_MAX;
		}else if (blend_mode == "GL_MIN"){
			bg.mode = GL_MIN;
		}else{
			bg.mode = GL_FUNC_ADD;
		}
		
		std::fill(bg.color, bg.color + 4, 0.0);
		bg.color[0] = wml::get_attr<GLfloat>(layer_node, "red", 1.0);
		bg.color[1] = wml::get_attr<GLfloat>(layer_node, "green", 1.0);
		bg.color[2] = wml::get_attr<GLfloat>(layer_node, "blue", 1.0);
		bg.color[3] = wml::get_attr<GLfloat>(layer_node, "alpha", 1.0);

		bg.y1 = wml::get_attr<int>(layer_node, "y1");
		bg.y2 = wml::get_attr<int>(layer_node, "y2");

		bg.foreground = wml::get_bool(layer_node, "foreground", false);
		layers_.push_back(bg);
	}
}

wml::node_ptr background::write() const
{
	wml::node_ptr res(new wml::node("background"));
	char buf[128];
	sprintf(buf, "%02x%02x%02x", top_.r, top_.g, top_.b);
	res->set_attr("top", buf);
	sprintf(buf, "%02x%02x%02x", bot_.r, bot_.g, bot_.b);
	res->set_attr("bottom", buf);
	res->set_attr("width", formatter() << width_);
	res->set_attr("height", formatter() << height_);

	foreach(const layer& bg, layers_) {
		wml::node_ptr node(new wml::node("layer"));
		node->set_attr("image", bg.image);
		node->set_attr("xscale", formatter() << bg.xscale);
		node->set_attr("yscale", formatter() << bg.yscale);
		node->set_attr("xspeed", formatter() << bg.xspeed);
		node->set_attr("yoffset", formatter() << bg.yoffset);
		node->set_attr("y1", formatter() << bg.y1);
		node->set_attr("y2", formatter() << bg.y2);
		node->set_attr("scale", formatter() << bg.scale);
		node->set_attr("red", formatter() << bg.color[0]);
		node->set_attr("green", formatter() << bg.color[1]);
		node->set_attr("blue", formatter() << bg.color[2]);
		node->set_attr("alpha", formatter() << bg.color[3]);

		if(bg.foreground) {
			node->set_attr("foreground", "true");
		}

		res->add_child(node);
	}
	return res;
}

void background::draw(double xpos, double ypos, int rotation, int cycle) const
{
	const int max_x = width_ - graphics::screen_width();
	const int max_y = height_ - graphics::screen_height();

	double x = xpos/max_x;
	double y = ypos/max_y;

	double y2 = y + double(graphics::screen_height())/double(height_);
	
	x = std::min<double>(1.0, std::max<double>(0.0, x));
	y = std::min<double>(1.0, std::max<double>(0.0, y));
	const GLfloat top_col[] = {top_.r/255.0,
	                           top_.g/255.0,
	                           top_.b/255.0};
	const GLfloat bot_col[] = {bot_.r/255.0,
	                           bot_.g/255.0,
	                           bot_.b/255.0};

	glShadeModel(GL_SMOOTH);
	glDisable(GL_TEXTURE_2D);
	glPushMatrix();
	glBegin(GL_POLYGON);

	glColor3fv(top_col);
	glVertex3i(xpos, ypos, 0);
	glVertex3i(xpos + graphics::screen_width(), ypos, 0);
	glColor3fv(bot_col);
	glVertex3i(xpos + graphics::screen_width(), ypos + height_, 0);
	glVertex3i(xpos, ypos + height_, 0);

	glEnd();

	if(rotation) {
		const int border = 100;
		glBegin(GL_POLYGON);
		glColor3fv(top_col);
		glVertex3i(-border, -border, 0);
		glVertex3i(graphics::screen_width() + border, -border, 0);
		glVertex3i(graphics::screen_width() + border, 0, 0);
		glVertex3i(-border, 0, 0);
		glEnd();

		glBegin(GL_POLYGON);
		glColor3fv(top_col);
		glVertex3i(-border, 0, 0);
		glVertex3i(graphics::screen_width() + border, 0, 0);
		glColor3fv(bot_col);
		glVertex3i(graphics::screen_width() + border, height_, 0);
		glVertex3i(-border, height_, 0);
		glEnd();
	}

	glColor3f(1.0,1.0,1.0);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);

	foreach(const layer& bg, layers_) {
		if(bg.foreground == false) {
			draw_layer(xpos, ypos, rotation, bg, cycle);
		}
	}
}

void background::draw_foreground(double xpos, double ypos, int rotation, int cycle) const
{
	foreach(const layer& bg, layers_) {
		if(bg.foreground) {
			draw_layer(xpos, ypos, rotation, bg, cycle);
		}
	}
}

void background::draw_layer(int x, int y, int rotation, const background::layer& bg, int cycle) const
{
	int screen_width = graphics::screen_width();

	//add some area around the edge of the screen to account for distortions.
	//TODO: find a more elegant and accurate way to do this.
	x -= 50;
	screen_width += 100;

	if(!bg.texture.valid()) {
		bg.texture = graphics::texture::get(bg.image, bg.image_formula);
		if(bg.y2 == 0) {
			bg.y2 = bg.texture.height();
		}
	}

	const double ScaleImage = 2.0;
	const double xscale = double(bg.xscale)/100.0;
	const double xpos = (-double(bg.xspeed)*double(cycle)/1000 + int(double(x)*xscale))/double(bg.texture.width()*ScaleImage);
	const double xpos2 = xpos + double(screen_width)/(bg.texture.width()*ScaleImage);
	
	glPushMatrix();
	glColor4fv(bg.color);
	glBlendEquation(bg.mode);
	graphics::blit_texture(bg.texture, x, y + bg.yoffset*ScaleImage - (y*bg.yscale)/100,
	                       screen_width,
					       (bg.y2 - bg.y1)*ScaleImage, 0.0, xpos,
						   double(bg.y1)/double(bg.texture.height()),
						   xpos2,
						   double(bg.y2)/double(bg.texture.height()));

	glColor4f(1.0,1.0,1.0,1.0);
	glBlendEquation(GL_FUNC_ADD);
	glPopMatrix();
}

