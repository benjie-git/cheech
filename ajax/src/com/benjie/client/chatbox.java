package com.benjie.client;

import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.ScrollPanel;
import com.google.gwt.user.client.ui.FocusPanel;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.ClickListener;
import com.google.gwt.user.client.ui.KeyboardListener;
import com.google.gwt.user.client.ui.MouseListener;
import com.google.gwt.user.client.ui.MouseListenerAdapter;
import com.google.gwt.user.client.ui.FocusListener;
import com.google.gwt.user.client.ui.FocusListenerAdapter;
import com.google.gwt.user.client.ui.KeyboardListenerAdapter;


public class ChatBox extends Composite
{
	private Cheech cheech;
	private final ScrollPanel chatArea= new ScrollPanel();
	private final TextBox chatTextBox = new TextBox();
	private final Button chatButton = new Button("Send");


	public ChatBox(Cheech ch)
	{
		cheech = ch;

		chatButton.addClickListener(new ClickListener()
		{
			public void onClick(Widget sender)
			{
				doSendChat();
			}
		});

		chatTextBox.addKeyboardListener(new KeyboardListenerAdapter()
		{
			public void onKeyPress(Widget sender, char keyCode, int modifiers)
			{
				if (keyCode == KEY_ENTER)
				{
					doSendChat();
				}
			}
		});

		VerticalPanel vp = new VerticalPanel();
		vp.setWidth("100%");
		vp.setSpacing(2);
		chatArea.setAlwaysShowScrollBars(true);
		chatArea.setWidth("100%");
		chatArea.setHeight("140px");
		chatArea.add(new VerticalPanel());
		FocusPanel fp = new FocusPanel(chatArea);
		vp.add(fp);

		fp.addClickListener(new ClickListener()
		{
			public void onClick(Widget sender)
			{
				focusChat();
			}
		});

		HorizontalPanel hp = new HorizontalPanel();
		hp.setWidth("100%");
		hp.setSpacing(4);
		hp.add(chatTextBox);
		chatTextBox.setVisibleLength(65);
		//chatTextBox.setWidth("100%");
		hp.add(chatButton);
		vp.add(hp);

		setWidget(vp);
		setStyleName("cheech-ChatBox");
		chatArea.setStyleName("cheech-ChatArea");
	}


	private void doSendChat()
	{
		if (chatTextBox.getText().length() > 0)
		{
			cheech.sendCommand("chat " + chatTextBox.getText());
			chatTextBox.setText("");
		}
	}


	public void focusChat()
	{
		chatTextBox.setFocus(true);
	}


	public void appendMessage(String msg)
	{
		Label label = new Label(msg);

		VerticalPanel vp = (VerticalPanel)(chatArea.getWidget(0));
		vp.add(label);
		chatArea.ensureVisible(label);
	}
}
