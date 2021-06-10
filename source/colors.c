#include "colors.h"

const char *c_error = "";
const char *c_normal = "";
const char *c_very_normal = "";
const char *c_red = "";
const char *c_blue = "";
const char *c_green = "";
const char *c_yellow = "";
const char *c_magenta = "";
const char *c_cyan = "";
const char *c_white = "";
const char *c_bright = "";

void set_colors(char nc)
{
	if (nc)
	{
		c_red = COLOR_ESCAPE "1";
		c_blue = COLOR_ESCAPE "2";
		c_green = COLOR_ESCAPE "3";
		c_yellow = COLOR_ESCAPE "4";
		c_magenta = COLOR_ESCAPE "5";
		c_cyan = COLOR_ESCAPE "6";
		c_white = COLOR_ESCAPE "7";

		c_bright = COLOR_ESCAPE "8";
		c_normal = COLOR_ESCAPE "9";

		c_very_normal = COLOR_ESCAPE "7" COLOR_ESCAPE "9";

		c_error = COLOR_ESCAPE "1";
	}
	else
	{
		c_red = "\033[31;40m";
		c_blue = "\033[34;40m";
		c_green = "\033[32;40m";
		c_yellow = "\033[33;40m";
		c_magenta = "\033[35;40m";
		c_cyan = "\033[36;40m";
		c_white = "\033[37;40m";

		c_bright = "\033[1;40m";
		c_normal = "\033[0;37;40m";

		c_very_normal = "\033[0m";

		c_error = "\033[1;4;40m";
	}
}
