package com.benjie.client;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.user.client.HTTPRequest;
import com.google.gwt.user.client.ResponseTextHandler;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.HTML;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.MenuBar;
import com.google.gwt.user.client.ui.MenuItem;
import com.google.gwt.user.client.ui.RootPanel;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.WindowCloseListener;
import com.google.gwt.user.client.Timer;
import com.google.gwt.user.client.Command;


public class Cheech implements EntryPoint 
{
	private final String BASE_URL = "cheechProxy.php";

	public final ChatBox chatBox = new ChatBox(this);
	public final Board board = new Board(this);
	public final Label status = new Label();

	private final Label moveCount = new Label();
	private final HTML gameInfo = new HTML();
	private final PlayerList playerList = new PlayerList(this);
	private InitDialog initDialog;
	private NameDialog nameDialog;
	private ColorDialog colorDialog;
	private SetupDialog setupDialog;

	private int clientId = 0;
	private int myPosn = 0;
	private int currentPlayer = 0;
	private int[] showList = null;
	private int numPlayers;
	private boolean longJumps;
	private boolean hopOthers;
	private boolean stopOthers;


	public void onModuleLoad()
	{
		Hole.preloadImages();

		VerticalPanel cheechPanel = new VerticalPanel();

		cheechPanel.add(createMenu());

		HorizontalPanel hp = new HorizontalPanel();
		hp.add(board);

		VerticalPanel vp = new VerticalPanel();
		vp.add(new HTML("<b>Game:</b>"));
		vp.add(gameInfo);
		gameInfo.setStyleName("cheech-GameInfo");
		vp.add(new HTML("<br/><hr/><br/><b>Players:</b>"));
		vp.add(playerList);
		playerList.setStyleName("cheech-PlayerInfo");
		vp.add(new HTML("<br/><hr/><br/><b>Move:</b>"));
		vp.add(moveCount);
		moveCount.setStyleName("cheech-MoveCount");
		hp.add(vp);
		vp.setStyleName("cheech-SidePanel");
		cheechPanel.add(hp);

		cheechPanel.add(new HTML("<hr/>"));

		cheechPanel.add(chatBox);

		cheechPanel.add(new HTML("<hr/>"));

		//HorizontalPanel statusp = new HorizontalPanel();
		//statusp.add(new HTML("<b>Status:</b>"));
		//statusp.add(status);
		//cheechPanel.add(statusp);

		RootPanel.get("cheech").add(cheechPanel);
		board.addButtons();

		Window.addWindowCloseListener(new WindowCloseListener()
		{
			public void onWindowClosed()
			{
				sendCommand("leave");
			}
			public String onWindowClosing() {return null;}
		});

		initDialog = new InitDialog(this);
		initDialog.show();
		initDialog.focusName();
	}


	public void sendCommand(String msg)
	{
		msg = msg.replace('&', '+');
		if (!HTTPRequest.asyncGet(BASE_URL + "?q=" + clientId + " " + msg,
								  new CheechResponseTextHandler()))
		{
			status.setText("Failed to connect.");
		}
	}


	private class CheechResponseTextHandler implements ResponseTextHandler
	{
		public void onCompletion(String responseText)
		{
			//chatBox.appendMessage(responseText + "\n");

			String arguments = "";
			int ind = responseText.indexOf(' ');
			if (ind != -1)
				arguments = responseText.substring(ind+1);

			if (responseText.startsWith("id"))
				onId(arguments);
			else if (responseText.startsWith("setposn"))
				onSetPosn(arguments);
			else if (responseText.startsWith("exit"))
				onExit();
			else if (responseText.startsWith("add"))
				onAdd(arguments);
			else if (responseText.startsWith("remove"))
				onRemove(arguments);
			else if (responseText.startsWith("board"))
				onBoard(arguments);
			else if (responseText.startsWith("rotate"))
				onRotate(arguments);
			else if (responseText.startsWith("info"))
				onInfo(arguments);
			else if (responseText.startsWith("chat"))
				onChat(arguments);
			else if (responseText.startsWith("show"))
				onShow(arguments);
			else if (responseText.startsWith("hide"))
				onHide();
			else if (responseText.startsWith("choose_name"))
				onChooseName(arguments);
			else if (responseText.startsWith("choose_color"))
				onChooseColor(arguments);
			else if (responseText.startsWith("move"))
				onMove(arguments);
			else if (responseText.startsWith("turn"))
				onTurn(arguments);
			else if (responseText.startsWith("finish"))
				onFinish(arguments);
			else if (responseText.startsWith("ping"))
				pollServer();
		}
	}


	private void onId(String arguments)
	{
		try
		{
			setClientId(Integer.parseInt(arguments));
			status.setText("got id.");
			initDialog.hide();
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed id");
			setClientId(0);
		}
		pollServer();
	}


	private void onSetPosn(String arguments)
	{
		try
		{
			myPosn = Integer.parseInt(arguments);
			status.setText("got my player number.");
		}
		catch (NumberFormatException nx)
		{
			myPosn = 0;
		}
		pollServer();
	}


	private void onExit()
	{
		status.setText("got exit.");
		pollServer();
	}


	private void onAdd(String arguments)
	{
		int posn;
		int color;
		String name;
		
		try
		{
			posn = Integer.parseInt(arguments.substring(0,1));
			color = Integer.parseInt(arguments.substring(2,3));
			name = arguments.substring(4);
			playerList.addPlayer(posn, name, color);
			board.setPlayerColor(posn, color);
			status.setText("got add.");
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed add");
		}
		pollServer();
	}


	private void onRemove(String arguments)
	{
		int posn;
		
		try
		{
			posn = Integer.parseInt(arguments);
			playerList.removePlayer(posn);
			board.setPlayerColor(posn, 0);
			status.setText("got remove.");
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed remove");
		}
		pollServer();
	}


	private void onBoard(String arguments)
	{
		// read in board hole states
		// 0=empty, 1-8=colors, 9=noHole
		try
		{
			int pos=0;

			for (int i=0; i<board.NumRows*board.NumCols; i++)
			{
				int player = Integer.parseInt(
								arguments.substring(pos++,pos++));
				if (player <= 6)
					board.addHole(i, player);
			}
			status.setText("got board info.");
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed board");
		}
		pollServer();
	}


	private void onRotate(String arguments)
	{
		try
		{
			int rotation = Integer.parseInt(arguments);
			board.rotateHoles(rotation);
			status.setText("got rotation.");
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed rotate");
		}
		pollServer();
	}


	private void onInfo(String arguments)
	{
		// set number of players, rules
		try
		{
			String infoStr = "";

			numPlayers = Integer.parseInt(arguments);
			longJumps = (Integer.parseInt(arguments.substring(2)) == 1);
			hopOthers = (Integer.parseInt(arguments.substring(6)) == 1);
			stopOthers = (Integer.parseInt(arguments.substring(4)) == 1);

			playerList.setNumPlayers(numPlayers);

			infoStr += "<b>" + numPlayers + " Player";
			if (numPlayers > 1) infoStr += "s";
			infoStr += "</b>";

			if (longJumps == true)
				infoStr += "<br/>Long Jumps";
			else
				infoStr += "<br/>No Long Jumps";
			if (hopOthers == false) infoStr += "<br/>No Throughs";
			if (stopOthers == false) infoStr += "<br/>No Stops";
			gameInfo.setHTML(infoStr);
			status.setText("got game info.");
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed num");
		}
		pollServer();
	}


	private void onChat(String arguments)
	{
		chatBox.appendMessage(arguments);
		status.setText("got msg.");
		pollServer();
	}


	private void onShow(String arguments)
	{
		// read in and make a move
		try
		{
			onHide();

			showList = parseMoveList(arguments);

			for (int i = 0; i < showList.length; i++)
				board.getHole(showList[i]).setSelected(true);
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed show");
		}
		pollServer();
	}


	private void onHide()
	{
		if (showList != null)
		{
			for (int i = 0; i < showList.length; i++)
				board.getHole(showList[i]).setSelected(false);
			showList = null;
		}
		pollServer();
	}


	private void onChooseName(String arguments)
	{
		String name;
		
		name = arguments;

		if (nameDialog == null)
		{
			nameDialog = new NameDialog(name, new Command ()
			{
				public void execute()
				{
					sendCommand("change_name " + nameDialog.getName());
					nameDialog.hide();
					nameDialog = null;
				}
			});

			nameDialog.show();
			nameDialog.focusName();
		}
		pollServer();
	}


	private void onChooseColor(String arguments)
	{
		String name;
		int color;

		try
		{
			color = Integer.parseInt(arguments);
			name = arguments.substring(2);

			if (colorDialog == null)
			{
				colorDialog = new ColorDialog(color, new Command ()
				{
					public void execute()
					{
						sendCommand("change_color " + colorDialog.getColor());
						colorDialog.hide();
						colorDialog = null;
					}
				});

				colorDialog.show();
			}
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed change_color");
		}
		pollServer();
	}


	private void onMove(String arguments)
	{
		// read in and make a move
		try
		{
			int moves[] = parseMoveList(arguments);

			onHide();
			board.getHole(moves[moves.length-1]).
				setPlayer(board.getHole(moves[0]).getPlayer());
			board.getHole(moves[0]).setPlayer(0);

			status.setText("got move.");
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed move");
		}
		pollServer();
	}


	private void onTurn(String arguments)
	{
		// read in turn info, hilight the current player, show new turn#
		try
		{
			int posn;
			int status;
			int count;

			posn = Integer.parseInt(arguments);
			status = Integer.parseInt(arguments.substring(2));
			count = Integer.parseInt(arguments.substring(4));

			currentPlayer = posn;
			playerList.hilight(currentPlayer);
			moveCount.setText(Integer.toString(count));

			if (currentPlayer == myPosn)
				getAttention();
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed move");
		}
		pollServer();
	}


	private void onFinish(String arguments)
	{
		// read in turn info, hilight the current player, show new turn#
		try
		{
			int posn;
			int move_count;

			posn = Integer.parseInt(arguments);
			move_count = Integer.parseInt(arguments.substring(2));

			playerList.finish(posn, move_count);

			if (currentPlayer == myPosn)
				getAttention();
		}
		catch (NumberFormatException nx)
		{
			chatBox.appendMessage("malformed finish");
		}
		pollServer();
	}


	private int[] parseMoveList(String arguments)
	{
		int pos = 0;
		int numMoves = Integer.parseInt(arguments);
		int moves[] = new int[numMoves];
	
		for (int i = 0; i < numMoves; i++)
		{
			pos = arguments.indexOf(' ', pos) + 1;
			moves[i] = Integer.parseInt(arguments.substring(pos));
		}

		return moves;
	}


	private void pollServer()
	{
		sendCommand("none");
	}
	

	public void setClientId(int id)
	{
		clientId = id;
	}
	

	private native void getAttention()
	/*-{
		window.focus();
	}-*/;


	private MenuBar createMenu()
	{
		MenuBar menu = new MenuBar();

		MenuBar gameMenu = new MenuBar(true);
		gameMenu.addItem("Join a Board...", new Command() {
			public void execute() {
				initDialog.setClientId(clientId);
				initDialog.show();
				initDialog.focusName();
			}
		});
		gameMenu.addItem("Change Game Settings...", new Command() {
			public void execute() {
				setupDialog = new SetupDialog(numPlayers, longJumps, hopOthers,
											  stopOthers, new Command ()
				{
					public void execute()
					{
						sendCommand("game_setup " + setupDialog.getParams());
						setupDialog.hide();
						setupDialog = null;
					}
				});
				setupDialog.show();
			}
		});

		gameMenu.addItem("<hr/>", true, new Command(){public void execute(){}});

		gameMenu.addItem("Undo Move", new Command() {
			public void execute() {
				sendCommand("undo");
			}
		});
		gameMenu.addItem("Redo Move", new Command() {
			public void execute() {
				sendCommand("redo");
			}
		});

		gameMenu.addItem("<hr/>", true, new Command(){public void execute(){}});

		gameMenu.addItem("Restart Game", new Command() {
			public void execute() {
				if (Window.confirm("This will end your current game.  Restart game anyway?"))
				{
					sendCommand("restart");
					Timer t = new Timer() { public void run() { pollServer(); } };
					t.schedule(1000);
				}
			}
		});
		gameMenu.addItem("Rotate Players", new Command() {
			public void execute() {
				if (Window.confirm("This will end your current game.  Rotate players anyway?"))
				{
					sendCommand("rotate");
					Timer t = new Timer() { public void run() { pollServer(); } };
					t.schedule(1000);
				}
			}
		});
		gameMenu.addItem("Shuffle Players", new Command() {
			public void execute() {
				if (Window.confirm("This will end your current game.  Shuffle players anyway?"))
				{
					sendCommand("shuffle");
					Timer t = new Timer() { public void run() { pollServer(); } };
					t.schedule(1000);
				}
			}
		});
		menu.addItem("Game", gameMenu);


		MenuBar playerMenu = new MenuBar(true);
		playerMenu.addItem("Change Name...", new Command() {
			public void execute() {
				nameDialog = new NameDialog("", new Command ()
				{
					public void execute()
					{
						sendCommand("change_name " + nameDialog.getName());
						nameDialog.hide();
						nameDialog = null;
					}
				});
				nameDialog.show();
				nameDialog.focusName();
			}
		});
		playerMenu.addItem("Change Color...", new Command() {
			public void execute() {
				colorDialog = new ColorDialog(0, new Command ()
				{
					public void execute()
					{
						sendCommand("change_color " + colorDialog.getColor());
						colorDialog.hide();
						colorDialog = null;
					}
				});
				colorDialog.show();
			}
		});

		MenuBar botMenu = new MenuBar(true);
		botMenu.addItem("Lookahead", new Command() {
			public void execute() {
				sendCommand("addbot l3");
			}
		});
		botMenu.addItem("Friendly", new Command() {
			public void execute() {
				sendCommand("addbot f3");
			}
		});
		botMenu.addItem("Mean", new Command() {
			public void execute() {
				sendCommand("addbot m3");
			}
		});
		playerMenu.addItem("Add Computer Player     >", botMenu);

		playerMenu.addItem("Remove Computer Players", new Command() {
			public void execute() {
				sendCommand("removebots");
			}
		});
		menu.addItem("Player", playerMenu);


		MenuBar helpMenu = new MenuBar(true);
		helpMenu.addItem("Playing Chinese Checkers...", new Command() {
			public void execute() {
				Window.open("chinesecheckers.html", "_blank", "");
			}
		});
		helpMenu.addItem("Using cheechweb...", new Command() {
			public void execute() {
				Window.open("cheechweb.html", "_blank", "");
			}
		});
		helpMenu.addItem("About cheech...", new Command() {
			public void execute() {
				Window.open("http://cheech.sourceforge.net/", "_blank", "");
			}
		});
		menu.addItem("Help", helpMenu);


		return menu;
	}
}
