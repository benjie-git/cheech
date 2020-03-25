package com.benjie.client;

import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Image;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.RadioButton;
import com.google.gwt.user.client.ui.Grid;
import com.google.gwt.user.client.ui.KeyboardListener;
import com.google.gwt.user.client.ui.KeyboardListenerAdapter;
import com.google.gwt.user.client.Command;


public class ColorPicker extends Composite {

	private Command callback;

	private final RadioButton redButton = new RadioButton("colors", "<img src=\"images/peg-red.png\"/>Red", true);
	private final RadioButton orangeButton = new RadioButton("colors", "<img src=\"images/peg-orange.png\"/>Orange", true);
	private final RadioButton yellowButton = new RadioButton("colors", "<img src=\"images/peg-yellow.png\"/>Yellow", true);
	private final RadioButton greenButton = new RadioButton("colors", "<img src=\"images/peg-green.png\"/>Green", true);
	private final RadioButton blueButton = new RadioButton("colors", "<img src=\"images/peg-blue.png\"/>Blue", true);
	private final RadioButton purpleButton = new RadioButton("colors", "<img src=\"images/peg-purple.png\"/>Purple", true);
	private final RadioButton blackButton = new RadioButton("colors", "<img src=\"images/peg-black.png\"/>Black", true);
	private final RadioButton whiteButton = new RadioButton("colors", "<img src=\"images/peg-white.png\"/>White", true);

	public ColorPicker(Command c) {
		callback = c;

		KeyboardListener onEnter = new KeyboardListenerAdapter()
		{
			public void onKeyPress(Widget sender, char keyCode, int modifiers)
			{
				if (keyCode == KEY_ENTER)
				{
					((RadioButton)sender).setChecked(true);

					callback.execute();
				}
			}
		};

		Grid colorp = new Grid(2, 4);
		colorp.setCellSpacing(4);
		colorp.setWidget(0, 0, redButton);
		redButton.addKeyboardListener(onEnter);
		colorp.setWidget(0, 1, orangeButton);
		orangeButton.addKeyboardListener(onEnter);
		colorp.setWidget(0, 2, yellowButton);
		yellowButton.addKeyboardListener(onEnter);
		colorp.setWidget(0, 3, greenButton);
		greenButton.addKeyboardListener(onEnter);
		colorp.setWidget(1, 0, blueButton);
		blueButton.addKeyboardListener(onEnter);
		colorp.setWidget(1, 1, purpleButton);
		purpleButton.addKeyboardListener(onEnter);
		colorp.setWidget(1, 2, blackButton);
		blackButton.addKeyboardListener(onEnter);
		colorp.setWidget(1, 3, whiteButton);
		whiteButton.addKeyboardListener(onEnter);

		setWidget(colorp);
	}


	public int getSelectedColor()
	{
		if (redButton.isChecked() == true)
			return 1;
		else if (orangeButton.isChecked() == true)
			return 2;
		else if (yellowButton.isChecked() == true)
			return 3;
		else if (greenButton.isChecked() == true)
			return 4;
		else if (blueButton.isChecked() == true)
			return 5;
		else if (purpleButton.isChecked() == true)
			return 6;
		else if (blackButton.isChecked() == true)
			return 7;
		else if (whiteButton.isChecked() == true)
			return 8;

		return 0;
	}


	public void setSelectedColor(int color)
	{
		
		if (color == 1)
			redButton.setChecked(true);
		else if (color == 2)
			orangeButton.setChecked(true);
		else if (color == 3)
			yellowButton.setChecked(true);
		else if (color == 4)
			greenButton.setChecked(true);
		else if (color == 5)
			blueButton.setChecked(true);
		else if (color == 6)
			purpleButton.setChecked(true);
		else if (color == 7)
			blackButton.setChecked(true);
		else if (color == 8)
			whiteButton.setChecked(true);
	}
}
