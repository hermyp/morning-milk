#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#define NES_HEADER_SIZE 16
#define NES_TRAINER_SIZE 512
#define NES_PRG_UNIT_SIZE 16384 // 16KB
#define NES_CHR_UNIT_SIZE 8192 // 8KB

#define NES_NES20_MASK 0x0C
#define NES_NES20_ID 0x08

#define NES_TRAINER_MASK 0x04
#define NES_TRAINER_ID 0x04

#define CHAR_EOF 0x1A

void check_format(std::vector<unsigned char> *data, bool *is_ines, bool *is_nes20)
{   *is_ines = false; *is_nes20 = false;
    if((*data)[0]=='N' && (*data)[1]=='E' && (*data)[2]=='S' && (*data)[3]==CHAR_EOF) *is_ines = true;
    if(*is_ines && ((*data)[7]&NES_NES20_MASK)==NES_NES20_ID) *is_nes20 = true;
}

void get_chr_loc(std::vector<unsigned char> *data, int *start, int *size)
{   int prg_size = (*data)[4] * NES_PRG_UNIT_SIZE;
    int trainer_size; if(((*data)[6]&NES_TRAINER_MASK)==NES_TRAINER_ID) trainer_size = NES_TRAINER_SIZE; else trainer_size = 0;
    *start = NES_HEADER_SIZE + trainer_size + prg_size;
    *size = (*data)[5] * NES_CHR_UNIT_SIZE;
}

#define NES_CHR_TILE_SIZE 16

class ChrTile {
public:
    ChrTile setData(std::vector<unsigned char> data)
    {   this->data = data;
        return *this;
    }
    std::vector<unsigned char> data;
};

unsigned char get_bit(unsigned char byte, int bit_number)
{   unsigned char result;
    switch(bit_number) {
    case 0:
        result = byte & 0b00000001;
        break;
    case 1:
        result = byte & 0b00000010;
        break;
    case 2:
        result = byte & 0b00000100;
        break;
    case 3:
        result = byte & 0b00001000;
        break;
    case 4:
        result = byte & 0b00010000;
        break;
    case 5:
        result = byte & 0b00100000;
        break;
    case 6:
        result = byte & 0b01000000;
        break;
    case 7:
        result = byte & 0b10000000;
        break;
    }
    result = result >> bit_number;
    return result;
}

void print_chr_tile(ChrTile tile)
{   for(int iy = 0; iy < 8; iy++)
    {   for(int ix = 7; ix >= 0; ix--)
        {   char colour;
            if(get_bit(tile.data[iy], ix)==0 && get_bit(tile.data[iy+8], ix)==0) colour = '.';
            if(get_bit(tile.data[iy], ix)==1 && get_bit(tile.data[iy+8], ix)==0) colour = '1';
            if(get_bit(tile.data[iy], ix)==0 && get_bit(tile.data[iy+8], ix)==1) colour = '2';
            if(get_bit(tile.data[iy], ix)==1 && get_bit(tile.data[iy+8], ix)==1) colour = '3';
            std::cout << colour;
        }
        std::cout << "\n";
    }
}

std::vector<ChrTile> get_chr_tiles(std::vector<unsigned char> *data, int start, int size)
{   int tile_size = size / NES_CHR_TILE_SIZE;
    std::vector<ChrTile> tiles;
    for(int i = 0; i < tile_size; i++)
    {   tiles.push_back(ChrTile().setData(std::vector<unsigned char>(data->begin() + (i*NES_CHR_TILE_SIZE + start), data->begin() + (i*NES_CHR_TILE_SIZE + start + NES_CHR_TILE_SIZE))));
    }
    return tiles;
}

#include <stb/stb_image_write.h>
#define SPRITESHEET_DIMENSION_SIZE 128
#define SPRITESHEET_DIMENSION_SIZE_IN_TILES 16

std::vector<unsigned char> compose_spritesheet_data_from_chr_tiles(std::vector<ChrTile> tiles)
{   std::vector<unsigned char> spritesheet_data = std::vector<unsigned char>(SPRITESHEET_DIMENSION_SIZE * SPRITESHEET_DIMENSION_SIZE * 3 + 256, 0); // added 256 because of malloc error
    for(int i = 0; i < tiles.size() && i < 256; i++)
    {   for(int iy = 0; iy < 8; iy++)
        {   for(int ix = 7; ix >= 0; ix--)
            {   unsigned char r, g, b;
                if(get_bit(tiles[i].data[iy], ix)==0 && get_bit(tiles[i].data[iy+8], ix)==0)
                {   r = 0x00; g = 0x00; b = 0x00;}
                if(get_bit(tiles[i].data[iy], ix)==1 && get_bit(tiles[i].data[iy+8], ix)==0)
                {   r = 0x80; g = 0x80; b = 0x80;}
                if(get_bit(tiles[i].data[iy], ix)==0 && get_bit(tiles[i].data[iy+8], ix)==1)
                {   r = 0xA9; g = 0xA9; b = 0xA9;}
                if(get_bit(tiles[i].data[iy], ix)==1 && get_bit(tiles[i].data[iy+8], ix)==1)
                {   r = 0xFF; g = 0xFF; b = 0xFF;}
                int x = i % SPRITESHEET_DIMENSION_SIZE_IN_TILES * 8 + (7 - ix);
                int y = i / SPRITESHEET_DIMENSION_SIZE_IN_TILES * 8 + iy;
                spritesheet_data[(y * SPRITESHEET_DIMENSION_SIZE + x) * 3 + 0] = r;
                spritesheet_data[(y * SPRITESHEET_DIMENSION_SIZE + x) * 3 + 1] = g;
                spritesheet_data[(y * SPRITESHEET_DIMENSION_SIZE + x) * 3 + 2] = b;
            }
        }
    }
    return spritesheet_data;
}

std::string compose_png_filename(std::string nes_rom_filename, int spritesheet_number)
{   return nes_rom_filename + "-" + std::to_string(spritesheet_number) + ".png";}

void save_png(std::string filename, std::vector<unsigned char> data)
{   stbi_write_png(filename.c_str(), SPRITESHEET_DIMENSION_SIZE, SPRITESHEET_DIMENSION_SIZE, 3, &data[0], SPRITESHEET_DIMENSION_SIZE * 3);}

std::string msg_file(std::string filename, std::string msg)
{   return filename + ": " + msg + "\n";}

#define MSG_NO_FILE "No file"
#define MSG_UNSUPPORTED_ROM_FORMAT "Unsupported file format. Expected iNES or NES 2.0 ROM"
#define MSG_TOO_SMALL_ROM "Too small"
void export_sprites_from_rom(std::string filename, bool use_8x16_mode)
{   std::ifstream file(filename, std::ios::binary);
    if(!file)
    {   std::cout << msg_file(filename, MSG_NO_FILE);
        return;
    }
    std::vector<unsigned char> data(std::istreambuf_iterator<char>(file), {});
    bool is_ines, is_nes20;
    if(data.size() < NES_HEADER_SIZE)
    {   std::cout << msg_file(filename, MSG_TOO_SMALL_ROM);
        return;
    }
    check_format(&data, &is_ines, &is_nes20);
    if(!is_ines)
    {   std::cout << msg_file(filename, MSG_UNSUPPORTED_ROM_FORMAT);
        return;
    }
    int chr_start, chr_size;
    get_chr_loc(&data, &chr_start, &chr_size);
    if(data.size() < chr_start + chr_size)
    {   std::cout << msg_file(filename, MSG_TOO_SMALL_ROM);
        return;
    }
    std::vector<ChrTile> tiles = get_chr_tiles(&data, chr_start, chr_size);
    int a = 0; if(tiles.size() % 256) a = 1;
    for(int i = 0; i < tiles.size() / 256 + a; i++)
    {   auto end = tiles.begin() + (i * 256) + 256;
        if(end >= tiles.end()) end = tiles.end();
        save_png(compose_png_filename(filename, i), compose_spritesheet_data_from_chr_tiles(std::vector<ChrTile>(tiles.begin() + (i * 256), end)));
    }
}

#define COMMAND_EXPORT "export"
bool arg_task_is_export(int argc, char **argv)
{   if(argc < 2) return false;
    if((std::string)COMMAND_EXPORT == argv[1]) return true;
    else return false;
}

std::vector<std::string> get_filenames_from_args(int argc, char **argv)
{   std::vector<std::string> filenames;
    for(int i = 2; i < argc; i++)
    {   filenames.push_back(argv[i]);
    }
    return filenames;
}

#define MSG_NO_EXPORT "No command or unsupported command is given"
#define MSG_NO_FILENAMES "No filenames are given"
int main(int argc, char **argv)
{   if(!arg_task_is_export(argc, argv))
    {   std::cout << MSG_NO_EXPORT << "\n";
        return 0;
    }
    std::vector<std::string> filenames = get_filenames_from_args(argc, argv);
    if(filenames.size() == 0)
    {   std::cout << MSG_NO_FILENAMES << "\n";
        return 0;
    }
    for(int i = 0; i < filenames.size(); i++)
    {   export_sprites_from_rom(filenames[i], false);
    }
}
