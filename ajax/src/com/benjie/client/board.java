package com.benjie.client;

import java.lang.Math;

import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.AbsolutePanel;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.ClickListener;
import com.google.gwt.user.client.ui.KeyboardListener;
import com.google.gwt.user.client.ui.KeyboardListenerAdapter;


public class Board extends Composite
{
	public static final int NumRows = 19;
	public static final int NumCols = 13;
	public static final double HoleWidth = 28.0;
	public static final double HoleHeight = HoleWidth * Math.sqrt(3.0) / 2.0;
	private final int[] center = {(int)(NumCols / 2 * HoleWidth),
								  (int)(NumRows / 2 * HoleHeight)};

	private AbsolutePanel ap = new AbsolutePanel();
	private Button sendButton = new Button("Make Move");
	private Button clearButton = new Button("Clear Move");
	private Hole[] holes;
	private int[] playerColors;
	private Cheech cheech;


	public Board(Cheech ch)
	{
		cheech = ch;

		holes = new Hole[NumCols*NumRows];
		playerColors = new int[6];

		ap.setPixelSize((int)(NumCols * HoleWidth) + 40,
						(int)((NumRows - 1) * HoleHeight));

		sendButton.addClickListener(new ClickListener()
		{
			public void onClick(Widget sender)
			{
				cheech.sendCommand("commit");
			}
		});

		sendButton.addKeyboardListener(new KeyboardListenerAdapter()
		{
			public void onKeyDown(Widget sender, char keyCode, int modifiers)
			{
				if (keyCode == KEY_ESCAPE)
					cheech.sendCommand("clear");

				else if (keyCode != KEY_ENTER)
					cheech.chatBox.focusChat();
			}
		});


		clearButton.addClickListener(new ClickListener()
		{
			public void onClick(Widget sender)
			{
				cheech.sendCommand("clear");
			}
		});

		setWidget(ap);

		setStyleName("cheech-Board");
	}


	public void addButtons()
	{
		VerticalPanel vp = new VerticalPanel();
		vp.add(clearButton);
		vp.add(sendButton);
		ap.add(vp);
		ap.setWidgetPosition(vp,
			ap.getAbsoluteLeft() + (int)(NumCols * HoleWidth) - 100,
			ap.getAbsoluteTop() + (int)(NumRows * HoleHeight) - 80);
	}


	public void addHole(int id, int player)
	{
		if (holes[id] != null)
		{
			holes[id].setPlayer(player);
			return;
		}

		holes[id] = new Hole(cheech, id);
		holes[id].setPlayer(player);

		int[] pos = getHolePosition(id);
		ap.setWidgetPosition(holes[id],
			pos[0] + center[0] + ap.getAbsoluteLeft() + 10,
			pos[1] + center[1]  + ap.getAbsoluteTop() - 10);

		ap.add(holes[id]);

		holes[id].setupListener();
	}


	public Hole getHole(int id)
	{
		return holes[id];
	}


	public void focusSendButton()
	{
		sendButton.setFocus(true);
	}


	public int getPlayerColor(int posn)
	{
		if (posn == 0)
			return 0;

		return playerColors[posn-1];
	}


	public void setPlayerColor(int posn, int color)
	{
		playerColors[posn-1] = color;

		for (int i=0; i<NumRows*NumCols; i++)
			if (holes[i] != null)
				holes[i].refresh();
	}


	private int[] getHolePosition(int id)
	{
		int[] pos = new int[2];

		int col = id % NumCols;
		int row = id / NumCols;
		int indent;

		if (row % 2 == 0)
			indent = (int)(HoleWidth / 2);
		else
			indent = 0;

		pos[0] = (int)(col * HoleWidth) + indent - center[0];
		pos[1] = (int)(row * HoleHeight) - center[1];

		return pos;
	}


	public void rotateHoles(int rot)
	{
		final double theta = (1-rot) * Math.PI / 3;
		final double cosTheta = Math.cos(theta);
		final double sinTheta = Math.sin(theta);
		int[] pos;
		int[] rotatedPos = new int[2];

		for (int i=0; i<NumRows*NumCols; i++)
		{
			if (getHole(i) != null)
			{
				pos = getHolePosition(i);

				rotatedPos[0] = (int)(0.5 + (pos[0] * cosTheta) - (pos[1] * sinTheta));
				rotatedPos[1] = (int)(0.5 + (pos[0] * sinTheta) + (pos[1] * cosTheta));

				ap.setWidgetPosition(holes[i],
					rotatedPos[0] + center[0] + ap.getAbsoluteLeft() + 10,
					rotatedPos[1] + center[1]  + ap.getAbsoluteTop() - 10);
			}
		}
	}
}
