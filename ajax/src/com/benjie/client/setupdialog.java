package com.benjie.client;

import com.google.gwt.user.client.ui.DialogBox;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.CheckBox;
import com.google.gwt.user.client.ui.ListBox;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.ClickListener;
import com.google.gwt.user.client.ui.KeyboardListener;
import com.google.gwt.user.client.ui.KeyboardListenerAdapter;
import com.google.gwt.user.client.Command;


public class SetupDialog extends DialogBox {

	private Command callback;

	private final ListBox numPlayers = new ListBox();
	private final CheckBox longJumps = new CheckBox("Allow Long Jumps");
	private final CheckBox hopOthers = new CheckBox("Allow Hopping Through Other Players' Triangles");
	private final CheckBox stopOthers = new CheckBox("Allow Stopping In Other Players' Triangles");
	private final Button restartButton = new Button("Restart Game");
	private final Button cancelButton = new Button("Cancel");

	public SetupDialog(int num, boolean longs, boolean hops, boolean stops, Command c) {
		callback = c;

		// Set the dialog box's caption.
		setText("Game Settings...");

		KeyboardListener onEnter = new KeyboardListenerAdapter()
		{
			public void onKeyPress(Widget sender, char keyCode, int modifiers)
			{
				if (keyCode == KEY_ENTER)
				{
					changeSettings();
				}
			}
		};

		VerticalPanel vp = new VerticalPanel();
		vp.setSpacing(10);

		HorizontalPanel nump = new HorizontalPanel();
		nump.setSpacing(10);
		nump.add(new Label("Number of Players:"));
		numPlayers.addItem("6 ");
		numPlayers.addItem("5 ");
		numPlayers.addItem("4 ");
		numPlayers.addItem("3 ");
		numPlayers.addItem("2 ");
		numPlayers.addItem("1 ");
		numPlayers.setSelectedIndex(6-num);
		numPlayers.addKeyboardListener(onEnter);
		nump.add(numPlayers);
		vp.add(nump);

		longJumps.addKeyboardListener(onEnter);
		if (longs == true)
			longJumps.setChecked(true);
		vp.add(longJumps);

		hopOthers.addKeyboardListener(onEnter);
		if (hops == true)
			hopOthers.setChecked(true);
		vp.add(hopOthers);

		stopOthers.addKeyboardListener(onEnter);
		if (stops == true)
			stopOthers.setChecked(true);
		vp.add(stopOthers);

		HorizontalPanel buttons = new HorizontalPanel();
		buttons.setSpacing(10);
		buttons.setWidth("100%");
		buttons.setHorizontalAlignment(HorizontalPanel.ALIGN_RIGHT);
		cancelButton.addClickListener(new ClickListener()
		{
			public void onClick(Widget sender)
			{
				hide();
			}
		});
		buttons.add(cancelButton);
		restartButton.addClickListener(new ClickListener()
		{
			public void onClick(Widget sender)
			{
				changeSettings();
			}
		});
		buttons.add(restartButton);
		vp.add(buttons);

		add(vp);
		setPopupPosition(50,100);
	}


	private void changeSettings()
	{
		callback.execute();
	}


	public String getParams()
	{
		return numPlayers.getItemText(numPlayers.getSelectedIndex())
			+ boolToStr(longJumps.isChecked())
			+ boolToStr(hopOthers.isChecked())
			+ boolToStr(stopOthers.isChecked());
	}


	private String boolToStr(boolean b)
	{
		if (b == true)
			return "1 ";
		else
			return "0 ";
	}
}
