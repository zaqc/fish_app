#include <gtkmm/window.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/glarea.h>
#include <gtk/gtk.h>
#include <vector>


#include <string>
#include <iostream>

#include <epoxy/gl.h>

class MainDrawingArea: public Gtk::DrawingArea {
public:
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;
};

bool MainDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	cr->set_source_rgb(0.0, 0.0, 0.25);
	cr->rectangle(0, 0, get_width(), get_height());
	cr->fill();
	return true;
}

class MainWindow: public Gtk::Window {
private:
	Gtk::VBox m_VBox;
	Gtk::HBox m_HBox;
	Gtk::Button m_ButtonQuit;
	Gtk::Button m_ButtonSend;
	//MainDrawingArea m_Draw;
	Gtk::GLArea m_GL;

	GLuint m_Vao { 0 };
	GLuint m_Buffer { 0 };
	GLuint m_Program { 0 };
	GLuint m_Mvp { 0 };

	std::vector<float> m_RotationAngles;

	void draw_triangle();
	bool GL_Render(const Glib::RefPtr<Gdk::GLContext> &context);

public:
	MainWindow();

	void init_shaders();
};

MainWindow::MainWindow() :
		m_VBox(), m_HBox(), m_ButtonQuit("Quit"), m_ButtonSend("Send"), m_GL() //, m_Draw()
{

	set_title("TuneApp");
	set_default_size(800, 600);

	m_HBox.pack_start(m_ButtonQuit, false, false, 4);
	m_HBox.pack_start(m_ButtonSend, false, false, 4);
	m_HBox.set_homogeneous(false);

	m_VBox.pack_start(m_HBox, false, false, 4);
	//m_VBox.pack_start(m_Draw, true, true, 4);
	m_VBox.pack_start(m_GL, true, true, 4);
	m_VBox.set_homogeneous(false);
	add(m_VBox);

	m_GL.signal_render().connect(sigc::mem_fun(*this, &MainWindow::GL_Render),
			false);
	//m_Draw.signal_draw().connect();

	show_all_children();
}

static GLuint create_shader(int type, const char *src) {
	auto shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		int log_len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

		std::string log_space(log_len + 1, ' ');
		glGetShaderInfoLog(shader, log_len, nullptr,
				(GLchar*) log_space.c_str());

		std::cerr << "Compile failure in "
				<< (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
				<< " shader: " << log_space << std::endl;

		glDeleteShader(shader);

		return 0;
	}

	return shader;
}

void MainWindow::init_shaders() {
	std::string vshader_bytes = "#version 330		\
			layout(location = 0) in vec4 position;	\
			uniform mat4 mvp;						\
			void main() {							\
				gl_Position = mvp * position;		\
			}";

	auto vertex = create_shader(GL_VERTEX_SHADER, (const char*) vshader_bytes.c_str());

	if (vertex == 0) {
		m_Program = 0;
		return;
	}

	std::string fshader_bytes = "#version 330	\
			out vec4 outputColor;				\
			void main() {						\
  				float lerpVal = gl_FragCoord.y / 500.0f;	\
  				outputColor = mix(vec4(1.0f, 0.85f, 0.35f, 1.0f), vec4(0.2f, 0.2f, 0.2f, 1.0f), lerpVal);	\
			}";

	auto fragment = create_shader(GL_FRAGMENT_SHADER, (const char*) fshader_bytes.c_str());

	if (fragment == 0) {
		glDeleteShader(vertex);
		m_Program = 0;
		return;
	}

	glAttachShader(m_Program, vertex);
	glAttachShader(m_Program, fragment);

	glLinkProgram(m_Program);

	int status;
	glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		int log_len;
		glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &log_len);

		std::string log_space(log_len + 1, ' ');
		glGetProgramInfoLog(m_Program, log_len, nullptr,
				(GLchar*) log_space.c_str());

		std::cerr << "Linking failure: " << log_space << std::endl;

		glDeleteProgram(m_Program);
		m_Program = 0;
	} else {
		/* Get the location of the "mvp" uniform */
		m_Mvp = glGetUniformLocation(m_Program, "mvp");

		glDetachShader(m_Program, vertex);
		glDetachShader(m_Program, fragment);
	}
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

enum {
	X_AXIS, Y_AXIS, Z_AXIS, N_AXIS
};

static const GLfloat vertex_data[] = { 0.f, 0.5f, 0.f, 1.f, 0.5f, -0.366f, 0.f,
		1.f, -0.5f, -0.366f, 0.f, 1.f, };

static void compute_mvp(float *res, float phi, float theta, float psi) {
	float x { phi * ((float) G_PI / 180.f) };
	float y { theta * ((float) G_PI / 180.f) };
	float z { psi * ((float) G_PI / 180.f) };
	float c1 { cosf(x) };
	float s1 { sinf(x) };
	float c2 { cosf(y) };
	float s2 { sinf(y) };
	float c3 { cosf(z) };
	float s3 { sinf(z) };
	float c3c2 { c3 * c2 };
	float s3c1 { s3 * c1 };
	float c3s2s1 { c3 * s2 * s1 };
	float s3s1 { s3 * s1 };
	float c3s2c1 { c3 * s2 * c1 };
	float s3c2 { s3 * c2 };
	float c3c1 { c3 * c1 };
	float s3s2s1 { s3 * s2 * s1 };
	float c3s1 { c3 * s1 };
	float s3s2c1 { s3 * s2 * c1 };
	float c2s1 { c2 * s1 };
	float c2c1 { c2 * c1 };

	/* apply all three rotations using the three matrices:
	 *
	 * ⎡  c3 s3 0 ⎤ ⎡ c2  0 -s2 ⎤ ⎡ 1   0  0 ⎤
	 * ⎢ -s3 c3 0 ⎥ ⎢  0  1   0 ⎥ ⎢ 0  c1 s1 ⎥
	 * ⎣   0  0 1 ⎦ ⎣ s2  0  c2 ⎦ ⎣ 0 -s1 c1 ⎦
	 */
	res[0] = c3c2;
	res[4] = s3c1 + c3s2s1;
	res[8] = s3s1 - c3s2c1;
	res[12] = 0.f;
	res[1] = -s3c2;
	res[5] = c3c1 - s3s2s1;
	res[9] = c3s1 + s3s2c1;
	res[13] = 0.f;
	res[2] = s2;
	res[6] = -c2s1;
	res[10] = c2c1;
	res[14] = 0.f;
	res[3] = 0.f;
	res[7] = 0.f;
	res[11] = 0.f;
	res[15] = 1.f;
}

void MainWindow::draw_triangle() {
	float mvp[16];

	compute_mvp(mvp, m_RotationAngles[X_AXIS], m_RotationAngles[Y_AXIS],
			m_RotationAngles[Z_AXIS]);

	glUseProgram(m_Program);

	glUniformMatrix4fv(m_Mvp, 1, GL_FALSE, &mvp[0]);

	glBindBuffer(GL_ARRAY_BUFFER, m_Vao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}

bool MainWindow::GL_Render(const Glib::RefPtr<Gdk::GLContext>& /* context */) {
	try {
		if(m_Program == 0)
			init_shaders();
		m_GL.throw_if_error();
		glClearColor(0.0, 0.0, 0.25, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		//draw_triangle();

		glFlush();

		return true;
	} catch (const Gdk::GLError &gle) {
//		cerr << "An error occurred in the render callback of the GLArea"
//				<< endl;
//		cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
		return false;
	}
}

int main(int argc, char **argv) {
	auto app = Gtk::Application::create(argc, argv, "TuneApp");

	MainWindow win;
	//win.init_shaders();

	return app->run(win);
}
