Rendering text in OpenGL can be a little challenging because OpenGL itself doesn't provide a built-in method for creating text. Instead, you have to use bitmaps or textures that contain your characters, and then render quads (or other shapes) with the appropriate portions of those bitmaps or textures applied.

You may also use libraries like FreeType or FTGL, which are designed to make this easier. 

Here's a basic guide on how to render text using FreeType and OpenGL:

Firstly, you need to include the necessary libraries. If you haven't installed the FreeType library, you need to do so.

```cpp
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Define a struct to represent each character as a texture:
struct Character {
    GLuint     TextureID;  // ID handle of the glyph texture
    glm::ivec2 Size;       // Size of glyph
    glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
    GLuint     Advance;    // Offset to advance to next glyph
};
std::map<GLchar, Character> Characters;

// Initialize FreeType library and load the font:
FT_Library ft;
if (FT_Init_FreeType(&ft))
    std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
FT_Face face;
if (FT_New_Face(ft, "path/to/font", 0, &face))
    std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
FT_Set_Pixel_Sizes(face, 0, 48); // set font size

// Load all the characters of the font:
glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
for (GLubyte c = 0; c < 128; c++) // load first 128 characters of ASCII set
{
    // Load character glyph 
    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
    {
        std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
        continue;
    }
    // generate texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        face->glyph->bitmap.width,
        face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        face->glyph->bitmap.buffer
    );
    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // now store character for later use
    Character character = {
        texture, 
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        static_cast<GLuint>(face->glyph->advance.x)
    };
    Characters.insert(std::pair<GLchar, Character>(c, character));
}

// To render the text:
// iterate through all characters
std::string::const_iterator c;
for (c = text.begin(); c != text.end(); c++) 
{
    Character ch = Characters[*c];
    GLfloat xpos = x + ch.Bearing.x * scale;
    GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
    GLfloat w = ch.Size.x * scale;
    GLfloat h = ch.Size.y * scale;
    // update VBO for each character
    GLfloat vertices[6][4] = {
        { xpos,     ypos + h,   0.0, 0.0 },            
        { xpos,     ypos,       0.0, 1.0 },
        { xpos + w, ypos,       1.0, 1.0 },

        { xpos,     ypos + h,   0.0, 0.0 },
        { xpos + w, ypos,       1.0, 1.0 },
        { xpos + w, ypos + h,   1.0, 0.0 }           
    };
    // render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, ch.TextureID);
    // update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // now advance cursors for next glyph
    x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
}
```

