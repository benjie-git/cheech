package com.benjie.client;

import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Image;
import com.google.gwt.user.client.DOM;
import com.google.gwt.user.client.Event;
import com.google.gwt.user.client.EventListener;


public class Hole extends Composite
{
	private Cheech cheech;
	private Image img = new Image();
	private int id;
	private int player = -1;

	public static final int NumColors = 8;
	private static final String[][] urls = {{"images/hole.png", "images/hole-hi.png"},
											{"images/peg-red.png", "images/peg-red-hi.png"},
											{"images/peg-orange.png", "images/peg-orange-hi.png"},
											{"images/peg-yellow.png", "images/peg-yellow-hi.png"},
											{"images/peg-green.png", "images/peg-green-hi.png"},
											{"images/peg-blue.png", "images/peg-blue-hi.png"},
											{"images/peg-purple.png", "images/peg-purple-hi.png"},
											{"images/peg-black.png", "images/peg-black-hi.png"},
											{"images/peg-white.png", "images/peg-white-hi.png"}};

	public Hole()
	{
		this(null, 0);
	}


	public Hole(Cheech ch, int id_)
	{
		cheech = ch;
		id = id_;
		setWidget(img);
	}


	public void setupListener()
	{
		DOM.sinkEvents(img.getElement(),
			Event.ONMOUSEDOWN + Event.ONDBLCLICK + Event.ONCLICK);

		DOM.setEventListener(img.getElement(), new EventListener() 
		{
			public void onBrowserEvent(Event ev)
			{
				if (DOM.eventGetType(ev) == Event.ONMOUSEDOWN)
				{
					cheech.sendCommand("click " + id);
					DOM.eventCancelBubble(ev, true);
				}
				else if (DOM.eventGetType(ev) == Event.ONCLICK)
				{
					cheech.board.focusSendButton();
				}
				else if (DOM.eventGetType(ev) == Event.ONDBLCLICK)
				{
					cheech.sendCommand("dblclick " + id);
					DOM.eventCancelBubble(ev, true);
				}
			}
		});
	}


	public static void preloadImages()
	{
		for (int i=0; i<NumColors; i++)
		{
			Image.prefetch(urls[i][0]);
			Image.prefetch(urls[i][1]);
		}
	}


	// For use as an image (like in PlayerList)
	public void setColor(int c)
	{
		img.setUrl(urls[c][0]);
	}


	public int getPlayer()
	{
		return player;
	}


	public void setPlayer(int p)
	{
		player = p;
		refresh();
	}


	public void refresh()
	{
		int color = cheech.board.getPlayerColor(player);
		img.setUrl(urls[color][0]);
	}


	public void setSelected(boolean selected)
	{
		int sel = 0;
		if (selected) sel = 1;

		int color = cheech.board.getPlayerColor(player);
		img.setUrl(urls[color][sel]);
	}
}
