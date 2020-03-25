package com.benjie.client;

import com.google.gwt.user.client.ui.DialogBox;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.ClickListener;
import com.google.gwt.user.client.ui.KeyboardListener;
import com.google.gwt.user.client.ui.KeyboardListenerAdapter;
import com.google.gwt.user.client.Command;


public class NameDialog extends DialogBox {

	private Command callback;

	private final TextBox nameEntry = new TextBox();
	private final Button changeButton = new Button("Change My Color");
	private final Button cancelButton = new Button("Cancel");

	public NameDialog(String name, Command c) {
		callback = c;

		// Set the dialog box's caption.
		setText("Change My Name To...");

		KeyboardListener onEnter = new KeyboardListenerAdapter()
		{
			public void onKeyPress(Widget sender, char keyCode, int modifiers)
			{
				if (keyCode == KEY_ENTER)
				{
					changeName();
				}
			}
		};

		VerticalPanel vp = new VerticalPanel();
		vp.setSpacing(10);

		HorizontalPanel namep = new HorizontalPanel();
		namep.setSpacing(10);
		namep.add(new Label("Name:"));
		namep.add(nameEntry);
		nameEntry.setText(name);
		nameEntry.addKeyboardListener(onEnter);
		vp.add(namep);

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
		changeButton.addClickListener(new ClickListener()
		{
			public void onClick(Widget sender)
			{
				changeName();
			}
		});
		buttons.add(changeButton);
		vp.add(buttons);

		add(vp);
		setPopupPosition(50,100);
	}


	private void changeName()
	{
		String name = nameEntry.getText();
		
		// Make sure a color is checked
		if (name.length() > 0)
		{
			callback.execute();
		}
	}


	public String getName()
	{
		return nameEntry.getText();
	}


	public void focusName()
	{
		nameEntry.setFocus(true);
	}
}
