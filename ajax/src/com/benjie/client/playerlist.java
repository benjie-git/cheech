package com.benjie.client;

import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Grid;


public class PlayerList extends Composite
{
	private Cheech cheech;
	private final Grid  grid = new Grid(6, 2);
	private int numPlayers = 0;
	private int lastHilighted = 0;


	public PlayerList(Cheech ch)
	{
		cheech = ch;

		setNumPlayers(6);

		setWidget(grid);
		setStyleName("cheech-ChatArea");
	}


	public void setNumPlayers(int n)
	{
		if (n < numPlayers)
		{
			for (int i=n; i<numPlayers; i++)
			{
				grid.clearCell(i, 0);
				grid.clearCell(i, 1);
			}
		}
		else
		{
			for (int i=numPlayers; i<n; i++)
			{
				Hole hole = new Hole();
				hole.setColor(0);
				grid.setWidget(i, 0, hole);
				
				grid.setText(i, 1, "");
			}
		}
		numPlayers = n;
	}


	public void addPlayer(int posn, String name, int color)
	{
		Hole hole = (Hole)(grid.getWidget(posn-1, 0));
		hole.setColor(color);
		grid.setText(posn-1, 1, name);
	}


	public void removePlayer(int posn)
	{
		Hole hole = (Hole)(grid.getWidget(posn-1, 0));
		hole.setColor(0);
		grid.setText(posn-1, 1, "");
	}


	public void hilight(int posn)
	{
		if (lastHilighted != 0)
			grid.getCellFormatter().setStyleName(lastHilighted-1, 1, "normal");

		if (posn != 0)
		{
			grid.getCellFormatter().setStyleName(posn-1, 1, "hilighted");
			removeCount(posn);
		}

		lastHilighted = posn;
	}


	public void finish(int posn, int move_count)
	{
		removeCount(posn);
		grid.setText(posn-1, 1, grid.getText(posn-1, 1) + " (" + move_count + ")");
	}


	private void removeCount(int posn)
	{
		String name = grid.getText(posn-1, 1);

		int ind = name.indexOf('(');
		if (ind != -1)
			grid.setText(posn-1, 1, name.substring(0, ind));
	}
}
