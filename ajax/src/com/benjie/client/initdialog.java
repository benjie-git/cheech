package com.benjie.client;

import com.google.gwt.user.client.ui.DialogBox;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.CheckBox;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.ClickListener;
import com.google.gwt.user.client.ui.KeyboardListener;
import com.google.gwt.user.client.ui.KeyboardListenerAdapter;
import com.google.gwt.user.client.Command;


public class InitDialog extends DialogBox {

	private Cheech cheech;
	private int clientId = 0;

	private final TextBox nameEntry = new TextBox();
	private ColorPicker colorPicker;
	private final CheckBox spectator = new CheckBox("Join as a spectator");
	private final Button joinButton = new Button("Join Board");
	private final Button cancelButton = new Button("Cancel");

	public InitDialog(Cheech ch) {
		cheech = ch;

		// Set the dialog box's caption.
		setText("Join cheech Board As...");

		KeyboardListener onEnter = new KeyboardListenerAdapter()
		{
			public void onKeyPress(Widget sender, char keyCode, int modifiers)
			{
				if (keyCode == KEY_ENTER)
				{
					join();
				}
			}
		};

		VerticalPanel vp = new VerticalPanel();
		vp.setSpacing(10);
		HorizontalPanel namep = new HorizontalPanel();
		namep.setSpacing(10);
		namep.add(new Label("Name:"));
		namep.add(nameEntry);
		nameEntry.addKeyboardListener(onEnter);
		vp.add(namep);

		vp.add(new Label("Color:"));
		colorPicker = new ColorPicker(new Command() 
			{ public void execute() { join(); } });
		vp.add(colorPicker);
		vp.add(spectator);
		spectator.addKeyboardListener(onEnter);

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
		joinButton.addClickListener( new ClickListener()
		{
			public void onClick(Widget sender)
			{
				join();
			}
		});
		buttons.add(joinButton);
		vp.add(buttons);

		add(vp);
		setPopupPosition(50,50);
	}


	private void join()
	{
		String name = nameEntry.getText();
		int color = colorPicker.getSelectedColor();
		
		// Make sure there's a name and a color is checked
		if (name.length() > 0 && (color > 0 || spectator.isChecked()))
		{
			if (clientId > 0)
			{
				cheech.sendCommand("leave");
				cheech.setClientId(0);
			}

			String specStr;
			if (spectator.isChecked() == true)
				specStr = "1";
			else
				specStr = "0";

			cheech.sendCommand("init " + color + " " + specStr + " " + name);

			cheech.status.setText("Looking for server...");
		}
	}


	public void focusName()
	{
		nameEntry.setFocus(true);
	}


	public void setClientId(int id)
	{
		clientId = id;
	}
}
