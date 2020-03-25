package com.benjie.client;

import com.google.gwt.user.client.ui.DialogBox;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.ClickListener;
import com.google.gwt.user.client.ui.KeyboardListener;
import com.google.gwt.user.client.ui.KeyboardListenerAdapter;
import com.google.gwt.user.client.Command;


public class ColorDialog extends DialogBox {

	private Command callback;

	private ColorPicker colorPicker;
	private final Button changeButton = new Button("Change My Color");
	private final Button cancelButton = new Button("Cancel");

	public ColorDialog(int color, Command c) {
		callback = c;

		// Set the dialog box's caption.
		setText("Change My Color To...");

		ClickListener onClick = new ClickListener()
		{
			public void onClick(Widget sender)
			{
				changeColor();
			}
		};

		VerticalPanel vp = new VerticalPanel();
		vp.setSpacing(10);

		vp.add(new Label("Color:"));
		colorPicker = new ColorPicker(new Command() 
			{ public void execute() { changeColor(); } });
		colorPicker.setSelectedColor(color);
		vp.add(colorPicker);

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
				changeColor();
			}
		});
		buttons.add(changeButton);
		vp.add(buttons);

		add(vp);
		setPopupPosition(50,100);
	}


	private void changeColor()
	{
		int color = colorPicker.getSelectedColor();
		
		// Make sure a color is checked
		if (color > 0)
		{
			callback.execute();
		}
	}


	public int getColor()
	{
		return colorPicker.getSelectedColor();
	}
}
