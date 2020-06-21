#define GRAPHICS_MAX_TEXTURES 4
#define GRAPHICS_MAX_SPRITES 64
#define GRAPHICS_MAX_SPRITE_FRAMES 256
#define GRAPHICS_MAX_NAME_LENGTH 32

typedef struct
{
	SDL_Point origin;
	SDL_Rect  bounds;
} SpriteFrame;

typedef struct 
{
	int maximum_frame_number;
	int current_frame_number;
	int frame_offset;
	int texture_index;
} Sprite;

/* Module internal state */
SDL_Renderer *renderer;

int          texture_count;
char         texture_names[GRAPHICS_MAX_TEXTURES][GRAPHICS_MAX_NAME_LENGTH];
SDL_Texture *texture_table[GRAPHICS_MAX_TEXTURES];

int          sprite_count;
char         sprite_names[GRAPHICS_MAX_SPRITES][GRAPHICS_MAX_NAME_LENGTH];
Sprite       sprite_table[GRAPHICS_MAX_SPRITES];

int          sprite_frame_count;
SpriteFrame  sprite_frame_table[GRAPHICS_MAX_SPRITE_FRAMES];

void graphics_init(SDL_Window* window, int width, int height)
{
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetLogicalSize(renderer, width, height);
	
	texture_count      = 0;
	sprite_count       = 0;
	sprite_frame_count = 0;
}

void graphics_quit(void)
{
	for(int i=texture_count-1; i >= 0; i--)
	{
		SDL_DestroyTexture(texture_table[i]);
		texture_count--;
	}
	
	SDL_DestroyRenderer(renderer);
}

void graphics_clear_screen(void)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderFillRect(renderer, NULL);
}

void graphics_update_screen(void)
{
	SDL_RenderPresent(renderer);
}

int graphics_load_texture(char *path)
{
	int texture_index = -1;
	
	/* Check if the texture is already in the texture table */
	for(int i=0; i < texture_count; i++)
	{
		if(strcmp(path, texture_names[i]) == 0)
		{
			texture_index = i;
			break;
		}
	}
	
	/* Otherwise, we actually load the texture */
	if(texture_index == -1)
	{
		if(texture_count > GRAPHICS_MAX_TEXTURES)
		{
			printf("graphics_load_texture: texture storage full");
		}
		else
		{
			SDL_Surface *tmp = SDL_LoadBMP(path);
			if(tmp == NULL)
			{
				printf("graphics_load_texture: can't find file \"%s\"\n", path);
			}
			else
			{
				/* First load the texture in the texture table */
				SDL_SetColorKey(tmp, SDL_TRUE, SDL_MapRGBA(tmp->format, 0, 162, 232, 255));
				texture_table[texture_count] = SDL_CreateTextureFromSurface(renderer, tmp);
				texture_index = texture_count;
				free(tmp);
				
				/* Then add its name to the texture name table */
				char* dst = texture_names[texture_count];
				int i;
				for(i=0; i < GRAPHICS_MAX_NAME_LENGTH-1; i++)
				{
					if(path[i] == '\0'); break;
					dst[i] = path[i];
				}
				dst[i] = '\0';
				
				/* Don't forget to note we got a new texture */
				texture_count++;
			}
		}
	}
	
	return texture_index;
}

int graphics_add_sprite(char *sprite_name, int frame_number, int texture_index)
{
	int sprite_index = -1;
	
	if((texture_index < 0) || (texture_index > texture_count - 1))
	{
		printf("graphics_add_sprite: invalid texture index\n");
	}
	else
	{
		if(sprite_count >= GRAPHICS_MAX_SPRITES)
		{
			printf("graphics_add_sprite: sprite limit of %d sprites has been reached\n", sprite_count);
		}
		else
		{
			if(sprite_frame_count + frame_number > GRAPHICS_MAX_SPRITE_FRAMES)
			{
				printf("graphics_add_sprite: not enough storage for the \"%s\" sprite frames\n", sprite_name);
			}
			else
			{
				/* First write sprite information in the sprite_table */
				Sprite *sprite = &sprite_table[sprite_count];
				
				sprite->current_frame_number = 0;
				sprite->maximum_frame_number = frame_number;
				sprite->frame_offset = sprite_frame_count;
				sprite->texture_index = texture_index;
				
				/* Then add its name to the sprite name table */
				int i;
				for(i=0; i < GRAPHICS_MAX_NAME_LENGTH-1; i++)
				{
					if(sprite_name[i] == '\0'); break;
					sprite_names[sprite_count][i] = sprite_name[i];
				}
				sprite_names[sprite_count][i] = '\0';
				
				/*Sprite is ready, we have a sprite index to return */
				sprite_index = sprite_count;
				
				/* Don't forget to note we got a new sprite and one or more new frames reserved for it */
				sprite_frame_count += frame_number;
				sprite_count++;
			}
		}
	}
	
	return sprite_index;
}

void graphics_add_sprite_frame(int sprite_index, int origin_x, int origin_y, int frame_x, int frame_y, int frame_width, int frame_height)
{
	if((sprite_index < 0) || (sprite_index > sprite_count - 1))
	{
		printf("graphics_add_sprite_frame: invalid sprite index\n");
	}
	else
	{
		Sprite *sprite = &sprite_table[sprite_index];
		
		if(sprite->current_frame_number >= sprite->maximum_frame_number)
		{
			printf("graphics_add_sprite_frame: maximum frame number of %d frames has been reached for this sprite\n", sprite->maximum_frame_number);
		}
		else
		{
			SpriteFrame *sprite_frame = &sprite_frame_table[sprite->frame_offset + sprite->current_frame_number];
			
			/* First write sprite information in the sprite_table */
			sprite_frame->origin.x = origin_x;
			sprite_frame->origin.y = origin_y;
			sprite_frame->bounds.x = frame_x;
			sprite_frame->bounds.y = frame_y;
			sprite_frame->bounds.w = frame_width;
			sprite_frame->bounds.h = frame_height;
			
			/* Don't forget to note the sprite got one more frame */
			sprite->current_frame_number++;
		}
	}
}

void graphics_draw_sprite(int sprite_index, int x, int y, int frame_index)
{
	if((sprite_index < 0) || (sprite_index >= sprite_count))
	{
		printf("graphics_draw_sprite: invalid sprite index\n");
		return;
	}
	
	Sprite *sprite = &sprite_table[sprite_index];
	
	if(sprite->current_frame_number == 0)
	{
		printf("graphics_draw_sprite: this sprite has no animation frames\n");
		return;
	}
	
	if(frame_index < 0)
	{
		frame_index = sprite->current_frame_number + (frame_index % sprite->current_frame_number);
	}
	else
	{
		frame_index %= sprite->current_frame_number;
	}
	
	SpriteFrame *sprite_frame = &sprite_frame_table[sprite->frame_offset + frame_index];
	
	SDL_Rect dst_rect;
	dst_rect.x = x - sprite_frame->origin.x;
	dst_rect.y = y - sprite_frame->origin.y;
	dst_rect.w = sprite_frame->bounds.w;
	dst_rect.h = sprite_frame->bounds.h;
	
	SDL_RenderCopy(renderer, texture_table[sprite->texture_index], &sprite_frame->bounds, &dst_rect);
}



