#include "command.h"

CommandSystem::CommandSystem(const std::string &file) : dataSystem(file)
{
	dataIO.open(file, std::ios::binary | std::ios::out | std::ios::in);
	Account = new AccountSystem("AccountSystem.txt");
	Finance = new FinanceSystem("FinanceSystem.txt");
	mainDatabase = new MainDatabase("Maindatabase.txt");
	ISBNDatabase = new IndexDatabase("ISBNDatabase.txt");
	nameDatabase = new IndexDatabase("nameDatabase.txt");
	authorDatabase = new IndexDatabase("authorDatabase.txt");
	keywordDatabase = new IndexDatabase("keywordDatabase.txt");
}

CommandSystem::~CommandSystem()
{
	dataIO.close();
	if (Account != nullptr) delete Account;
	if (Finance != nullptr) delete Finance;
	if (mainDatabase != nullptr) delete mainDatabase;
	if (ISBNDatabase != nullptr) delete ISBNDatabase;
	if (nameDatabase != nullptr) delete nameDatabase;
	if (authorDatabase != nullptr) delete authorDatabase;
	if (keywordDatabase != nullptr) delete keywordDatabase;
}

std::vector<std::string> CommandSystem::parse(const std::string &str)
{
	std::vector<std::string> ret;
	int quote = 0;
	size_t pre = -1;
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == '"') quote++;
		if (str[i] == ' ' && quote % 2 == 0)
		{
			ret.push_back(str.substr(pre + 1, i - pre - 1));
			pre = i;
		}
	}
	if (pre != str.length() - 1) ret.push_back(str.substr(pre + 1, str.length() - 1 - pre));
	return ret;
}

void CommandSystem::erase(DataType data)
{
	ISBNDatabase->erase(data.ISBN, data.ISBN);
	nameDatabase->erase(data.name, data.ISBN);
	authorDatabase->erase(data.author, data.ISBN);

	std::string keyword = data.keyword;
	while (keyword.find("|") != std::string::npos)
	{
		data.keyword = keyword.substr(0, keyword.find("|"));
		keywordDatabase->erase(data.keyword, data.ISBN);
		keyword = keyword.substr(keyword.find("|") + 1, keyword.length() - 1 - keyword.find("|"));
	}
	data.keyword = keyword;
	keywordDatabase->erase(data.keyword, data.ISBN);
}

void CommandSystem::modify(DataType old, DataType data)
{
	erase(old);
	int address = mainDatabase->printToBack(data.printToString());

	ISBNDatabase->write(data.ISBN, data.ISBN, IndexType(data.ISBN, data.ISBN, address).printToString());
	nameDatabase->write(data.name, data.ISBN, IndexType(data.name, data.ISBN, address).printToString());
	authorDatabase->write(data.author, data.ISBN, IndexType(data.author, data.ISBN, address).printToString());

	std::string keyword = data.keyword;
	while (keyword.find("|") != std::string::npos)
	{
		data.keyword = keyword.substr(0, keyword.find("|"));
		keywordDatabase->write(data.keyword, data.ISBN, IndexType(data.keyword, data.ISBN, address).printToString());
		keyword = keyword.substr(keyword.find("|") + 1, keyword.length() - 1 - keyword.find("|"));
	}
	data.keyword = keyword;
	keywordDatabase->write(data.keyword, data.ISBN, IndexType(data.keyword, data.ISBN, address).printToString());
}

void CommandSystem::printSelected()
{
	for (auto u : curSelected)
	{
		std::cout.setf(std::ios::fixed);
		std::cout << u.ISBN << "\t" << u.name << "\t" << u.author << "\t" << u.keyword << "\t";
		std::cout << std::setprecision(2) << u.price << "\t" << u.quantity << "本" << "\n";
	}
}

void CommandSystem::cleanup()
{
	ISBNDatabase->cleanup();
	nameDatabase->cleanup();
	authorDatabase->cleanup();
	keywordDatabase->cleanup();
}

ResultType CommandSystem::userCommand(std::vector<std::string> token)
{
	std::string cmd = token[0];

	if (cmd == "su")
	{
		if (token.size() == 3) Account->login(token[1], token[2]);
		else if (token.size() == 2) Account->login(token[1]);
		else throw std::logic_error("Invalid");
	}
	else if (cmd == "logout")
	{
		if (token.size() != 1) throw std::logic_error("Invalid");
		Account->logout();
	}
	else if (cmd == "useradd")
	{
		if (token.size() != 5) throw std::logic_error("Invalid");
		Account->add(token[3][0] - '0', token[1], token[2], token[4]);
	}
	else if (cmd == "register")
	{
		if (token.size() != 4) throw std::logic_error("Invalid");
		Account->addRegister(token[1], token[2], token[3]);
	}
	else if (cmd == "delete")
	{
		if (token.size() != 2) throw std::logic_error("Invalid");
		Account->erase(token[1]);
	}
	else if (cmd == "passwd")
	{
		if (token.size() == 3) Account->changePassword(token[1], token[2]);
		else if (token.size() == 4) Account->changePassword(token[1], token[3], token[2]);
		else throw std::logic_error("Invalid");
	}
	return Executed;
}

ResultType CommandSystem::dataCommand(std::vector<std::string> token)
{
	std::string cmd = token[0];

	if (cmd == "select")
	{
		if (!(Account->curLevel >= 3)) throw std::logic_error("Invalid");
		curSelected.clear();
		int s = ISBNDatabase->read(token[1], token[1]);
		curSelected.push_back((s != 0) ? mainDatabase->read(s) : DataType());
		if (curSelected.size() != 1) throw std::logic_error("Invalid");
		curSelected[0].ISBN = token[1]; //set default ISBN number, also apply when a book has already been created
	}
	else if (cmd == "modify")
	{
		if (!(Account->curLevel >= 3)) throw std::logic_error("Invalid");
		if (curSelected.size() != 1) throw std::logic_error("Invalid");
		DataType &t = curSelected[0], backup = curSelected[0];
		for (size_t i = 1; i < token.size(); i++)
		{
			if (token[i].substr(0, 5) == "-ISBN")
			{
				t.ISBN = token[i].substr(6, token[i].length() - 6);
				if (ISBNDatabase->read(t.ISBN, t.ISBN)) throw std::logic_error("Invalid");
			}
			else if (token[i].substr(0, 5) == "-name")
			{
				t.name = token[i].substr(7, token[i].length() - 2 - 7 + 1);
			}
			else if (token[i].substr(0, 7) == "-author")
			{
				t.author = token[i].substr(9, token[i].length() - 2 - 9 + 1);
			}
			else if (token[i].substr(0, 8) == "-keyword")
			{
				t.keyword = token[i].substr(10, token[i].length() - 2 - 10 + 1);
			}
			else if (token[i].substr(0, 6) == "-price")
			{
				t.price = stringToDouble(token[i].substr(7, token[i].length() - 7));
			}
		}
		modify(backup, t);
	}
	else if (cmd == "import")
	{
		if (!(Account->curLevel >= 3)) throw std::logic_error("Invalid");
		if (curSelected.size() != 1) throw std::logic_error("Invalid");
		DataType &t = curSelected[0], backup = curSelected[0];
		t.quantity += stringToInteger(token[1]);
		modify(backup, t); //to be optimized
		Finance->addEvent(stringToInteger(token[1]), stringToDouble(token[2]), false);
	}
	else if (cmd == "show" && token.size() == 1)
	{
		if (!(Account->curLevel >= 1)) throw std::logic_error("Invalid");
		auto && tmp = ISBNDatabase->readAll("");
		curSelected.clear();
		for (auto t : tmp) curSelected.push_back(mainDatabase->read(t));
		printSelected();
	}
	else if (cmd == "show" && token.size() > 1 && token[1] != "finance")
	{
		if (!(Account->curLevel >= 1)) throw std::logic_error("Invalid");
		if (token.size() != 2) throw std::logic_error("Invalid");
		curSelected.clear();
		std::vector<int> tmp;
		if (token[1].substr(0, 5) == "-ISBN")
		{
			tmp = ISBNDatabase->readAll(token[1].substr(6, token[1].length() - 6));
		}
		else if (token[1].substr(0, 5) == "-name")
		{
			tmp = nameDatabase->readAll(token[1].substr(7, token[1].length() - 2 - 7 + 1));
		}
		else if (token[1].substr(0, 7) == "-author")
		{
			tmp = authorDatabase->readAll(token[1].substr(9, token[1].length() - 2 - 9 + 1));
		}
		else if (token[1].substr(0, 8) == "-keyword")
		{
			tmp = keywordDatabase->readAll(token[1].substr(10, token[1].length() - 2 - 10 + 1));
		}
		for (auto t : tmp) curSelected.push_back(mainDatabase->read(t));
		printSelected();
	}
	else if (cmd == "show" && token[1] == "finance")
	{
		if (!(Account->curLevel >= 7)) throw std::logic_error("Invalid");
		if (token.size() == 2) Finance->printEvent();
		else Finance->printEvent(stringToInteger(token[2]));
	}
	else if (cmd == "buy")
	{
		if (!(Account->curLevel >= 1)) throw std::logic_error("Invalid");
		int num = ISBNDatabase->read(token[1], token[1]);
		if (num == 0) throw std::logic_error("Invalid");
		DataType t = mainDatabase->read(num), backup = t;
		int quantity = stringToInteger(token[2]);
		if (t.quantity < quantity) throw std::logic_error("Invalid");
		t.quantity -= quantity;
		modify(backup, t); // to be optimized
		Finance->addEvent(quantity, t.price * quantity, true);
	}
	else throw std::logic_error("Invalid");
	cleanup();
	return Executed;
}

ResultType CommandSystem::runCommand(const std::string &str)
{
	auto token = parse(str);

	std::string cmd = token[0];

	if (cmd == "exit") return Exit;
	if (cmd == "load") return runLoadCommand(token[1]);

	if (cmd == "su" || cmd == "logout" || cmd == "useradd" || cmd == "register" || cmd == "delete" || cmd == "passwd")
		return userCommand(token);

	if (cmd == "select" || cmd == "modify" || cmd == "import" || cmd == "show" || cmd == "buy")
		return dataCommand(token);

	throw std::logic_error("Invalid");
}

ResultType CommandSystem::runLoadCommand(const std::string &file)
{
	std::ifstream fileCommandIO(file, std::ios::binary | std::ios::in);
	char r[maxCommandLen];
	while (fileCommandIO.getline(r, maxCommandLen))
	{
		std::string str = r;
		while (str[str.length() - 1] == '\r') str = str.substr(0, str.length() - 1);
		try
		{
			//std::cerr << str << std::endl;
			auto t = runCommand(str);
			if (t == Exit) return Exit;
		}
		catch (std::logic_error &error)
		{
			std::cout << error.what() << std::endl;
		}
	}
	return Executed;
}
