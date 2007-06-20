#include "subtle.h"

 /**
	* Create a new tile 
	* @param mode Tile mode
	* @return Return either a #SubWin on success or otherwise NULL.
	**/

SubWin *
subTileNew(short mode)
{
	SubWin *w = subWinNew(0);

	w->flags	= SUB_WIN_TYPE_TILE;
	w->flags |= mode;
	w->tile		= (SubTile *)calloc(1, sizeof(SubTile));
	if(!w->tile) subLogError("Can't alloc memory. Exhausted?\n");

	w->tile->btnew	= XCreateSimpleWindow(d->dpy, w->frame, 
		d->th, 0, 7 * d->fx, d->th, 0, d->colors.border, d->colors.norm);
	w->tile->btdel	= XCreateSimpleWindow(d->dpy, w->frame, 
		d->th + 7 * d->fx, 0, 7 * d->fx, d->th, 0, d->colors.border, d->colors.norm);

	subWinMap(w);
	subLogDebug("Adding %s-tile: x=%d, y=%d, width=%d, height=%d\n", 
		(w->flags & SUB_WIN_TILE_HORZ) ? "h" : "v", w->x, w->y, w->width, w->height);

	return(w);
}

 /**
	* Delete a tile window and all children 
	* @param w A #SubWin
	**/

void
subTileDelete(SubWin *w)
{
	if(w && w->flags & SUB_WIN_TYPE_TILE)
		{
			unsigned int n, i;
			Window nil, *wins = NULL;

			XGrabServer(d->dpy);
			subWinUnmap(w);

			XQueryTree(d->dpy, w->win, &nil, &nil, &wins, &n);	
			for(i = 0; i < n; i++)
				{
					SubWin *c = subWinFind(wins[i]);
					if(c && c->flags & SUB_WIN_TYPE_TILE) subTileDelete(c);
					else if(c && c->flags & SUB_WIN_TYPE_CLIENT) subClientSendDelete(c);
				}

			subLogDebug("Deleting %s-tile with %d children\n", (w->flags & SUB_WIN_TILE_HORZ) ? "h" : "v", n);

			free(w->tile); 
			subWinDelete(w);
			XFree(wins);

			XSync(d->dpy, False);
			XUngrabServer(d->dpy);
		}
}

 /**
	* Render a tile window 
	* @param mode Render mode
	* @param w A #SubWin
	**/

void
subTileRender(short mode,
	SubWin *w)
{
	if(w)
		{
			unsigned long col = mode ? (w->flags & SUB_WIN_OPT_COLLAPSE ? d->colors.cover : d->colors.norm) : d->colors.focus;

			if(w->flags & SUB_WIN_TYPE_TILE)
				{
					XSetWindowBackground(d->dpy, w->tile->btnew, col);
					XSetWindowBackground(d->dpy, w->tile->btdel, col);
					XClearWindow(d->dpy, w->tile->btnew);
					XClearWindow(d->dpy, w->tile->btdel);

					/* Descriptive buttons */
					XDrawString(d->dpy, w->tile->btnew, d->gcs.font, 3, d->fy - 1, 
						(w->flags & SUB_WIN_TILE_HORZ ? "Newrow" : "Newcol"), 6);
					XDrawString(d->dpy, w->tile->btdel, d->gcs.font, 3, d->fy - 1, 
						(w->parent && w->parent->flags & SUB_WIN_TILE_HORZ ? "Delrow" : "Delcol"), 6);
				}
		}
}

 /**
	* Add a window to a tile 
	* @param t A #SubWin
	* @param w A #SubWin
	**/

void
subTileAdd(SubWin *t,
	SubWin *w)
{
	if(t && w)
		{
			XReparentWindow(d->dpy, w->frame, t->win, 0, 0);
			subWinRestack(w);
			subTileConfigure(t);
			w->parent = t;

			subLogDebug("Adding window: x=%d, y=%d, width=%d, height=%d\n", w->x, w->y, w->width, w->height);
		}
}

 /**
	* Configure a tile and each child 
	* @param w A #SubWin
	**/

void
subTileConfigure(SubWin *w)
{
	if(w && w->flags & SUB_WIN_TYPE_TILE)
		{
			int mw = 0, mh = 0, width = SUBWINWIDTH(w), height = SUBWINHEIGHT(w);
			unsigned int x = 0, y = 0, i = 0, n = 0, size = 0, comp = 0, collapsed = 0, weighted = 0;
			Window nil, *wins = NULL;

			XQueryTree(d->dpy, w->win, &nil, &nil, &wins, &n);
			if(n > 0)
				{
					size = (w->flags & SUB_WIN_TILE_HORZ) ? width : height;

					/* Find weighted and collapsed windows */
					for(i = 0; i < n; i++)
						{
							SubWin *c = subWinFind(wins[i]);
							if(c)
								{
									if(c->flags & SUB_WIN_OPT_COLLAPSE) collapsed++;
									else if(c->flags & SUB_WIN_OPT_WEIGHT && c->weight > 0)
										{
											if(n == 1)
												{
													c->flags &= ~SUB_WIN_OPT_WEIGHT;
													c->weight = 0;
												}
											else size -= size * c->weight / 100;
											weighted++;
										}
								}
						}

					if(weighted > 0)
						{
							if(w->flags & SUB_WIN_TILE_HORZ) width = size;
							else if(w->flags & SUB_WIN_TILE_VERT) height = size;
						}

					n		= (n - collapsed - weighted) > 0 ? n - collapsed - weighted : 1; /* Prevent divide by zero */
					mw	= (w->flags & SUB_WIN_TILE_HORZ) ? width / n : width;
					mh	= (w->flags & SUB_WIN_TILE_VERT) ? (height - collapsed * d->th) / n : height;

					/* Get compensation for bad rounding */
					if(w->flags & SUB_WIN_TILE_HORZ) comp = abs(width - n * mw - collapsed * d->th);
					else comp = abs(height - n * mh - collapsed * d->th);

					for(i = 0; i < n + collapsed + weighted; i++)
						{
							SubWin *c = subWinFind(wins[i]);
							if(c && !(c->flags & (SUB_WIN_OPT_RAISE|SUB_WIN_OPT_TRANS)))
								{
									c->parent	= w;
									c->x			= (w->flags & SUB_WIN_TILE_HORZ) ? x : 0;
									c->y			= (w->flags & SUB_WIN_TILE_VERT) ? y : 0;
									c->width	= (w->flags & SUB_WIN_TILE_HORZ && c->flags & SUB_WIN_OPT_WEIGHT) ? 
										SUBWINWIDTH(w) * c->weight / 100 : mw;
									c->height = (c->flags & SUB_WIN_OPT_COLLAPSE) ? d->th : 
										((w->flags & SUB_WIN_TILE_VERT && c->flags & SUB_WIN_OPT_WEIGHT) ? 
										SUBWINHEIGHT(w) * c->weight / 100 : mh);

									/* Add compensation to width or height */
									if(i == n + collapsed + weighted - 1) 
										if(w->flags & SUB_WIN_TILE_HORZ) c->width += comp; 
										else c->height += comp;
		
									x += c->width;
									y += c->height;

									subLogDebug("Configuring %s-window: x=%d, y=%d, width=%d, height=%d, weight=%d\n", 
										(c->flags & SUB_WIN_OPT_COLLAPSE) ? "c" : ((w->flags & SUB_WIN_OPT_WEIGHT) ? "w" : "n"), 
										c->x, c->y, c->width, c->height, c->weight);

									subWinResize(c);
									if(w->flags & SUB_WIN_TYPE_TILE) subTileConfigure(c);
									else if(w->flags & SUB_WIN_TYPE_CLIENT) subClientSendConfigure(c);
								}
						}
					XFree(wins);
					subLogDebug("Configuring %s-tile: n=%d, mw=%d, mh=%d\n", (w->flags & SUB_WIN_TILE_HORZ) ? "h" : "v", n, mw, mh);
				}
		}
}
