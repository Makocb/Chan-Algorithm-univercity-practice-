#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <ctime>

using namespace std;

struct Dot // точка на плоскости
{
	double x, y;
	Dot(const double a = 0, const double b = 0) : x(a), y(b) {}
};

/*
         | i  j  k |
[a, b] = | xa ya 0 | = { 0, 0, xa * yb - ya * xb }
         | xb yb 0 |
*/

struct FlatVec // вектор на плоскости
{
	double x, y;

	FlatVec() : x(), y() {}

	void reFill(const Dot &dot1, const Dot &dot2) // построение по двум точкам
	{
		x = dot2.x - dot1.x;
		y = dot2.y - dot1.y;
	}

	FlatVec(const Dot &dot1, const Dot &dot2) { reFill(dot1, dot2); }
};

// истина, если кратчайший поворот от vec1 к vec2 видится против часовой стрелки
bool isright(const FlatVec& vec1, const FlatVec& vec2)
{
	return vec1.x * vec2.y >= vec1.y * vec2.x;
}

using Polygon = vector<Dot>; // набор точек, т.е. многоугольник

// создание случайного многоугольника
Polygon randpol(const size_t len, const double low = -100.0, const double high = 100.0)
{
	Polygon res;
	for (size_t i = 0; i < len; i++)
		res.emplace_back(
			low + (double)rand() / RAND_MAX * (high - low),
			low + (double)rand() / RAND_MAX * (high - low)
		);
	return res;
}

istream &operator>>(istream &in, Polygon &pol) // ввод многоугольника
{
	size_t len;
	in >> len;
	double x, y;
	for (size_t i = 0; i < len; i++)
	{
		in >> x >> y;
		pol.emplace_back(x, y);
	}
	return in;
}

ostream &operator<<(ostream &out, const Polygon &pol) // вывод многоугольника
{
	out << pol.size();
	for (const Dot &e : pol) out << endl << e.x << " " << e.y;
	return out;
}

Polygon Graham(Polygon pol, bool issorted = false) // алгоритм Грэхема
{
	FlatVec vec1, vec2;
	/* предварительная сортировка по полярному углу 
	относительно нижней левой точки */
	if (!issorted)
	{
		auto c = pol.begin(); // находим нижнюю левую точку
		for (auto it = pol.begin() + 1; it != pol.end(); ++it)
			if (it->y <  c->y || // если очередная точка ниже точки pol[c]
				it->y == c->y && // или если на одном уровне, но левее
				it->x <  c->x) c = it; // то выбираем её

		iter_swap(pol.begin(), c); // ставим нижнюю левую точку в начало

		sort( // сортируем по полярному углу, т.е. по направлению кратчайшего поворота
			pol.begin() + 1,
			pol.end(),
			[&pol, &vec1, &vec2](const Dot &dot1, const Dot &dot2)
			{
				vec1.reFill(pol.front(), dot1);
				vec2.reFill(pol.front(), dot2);
				return isright(vec1, vec2);
			}
		);
	}

	// отсеиваем точки по алгоритму Грэхема
	for (auto i = next(pol.begin()), j = next(i), k = next(j); k != pol.end();)
	{
		//if (k == pol.end()) k = pol.begin(); // при последней итерации ломаная линия замкнётся
		vec1.reFill(*i, *j); vec2.reFill(*j, *k);
		if (isright(vec1, vec2)) { ++i; ++j; ++k; }
		else
		{
			--i; --j; advance(k, -2); // итератор не должен указывать
			pol.erase(next(k, 1)); ++k; // на удаляемый элемент, иначе он станет недействительным
		}
	}

	return pol;
}

vector<Polygon> Chan(const Polygon &pol, const size_t amount)
{
	vector<Polygon> res;
	const size_t len = pol.size();
	const size_t sep_len = len / amount;
	Polygon separated_pol;

	for (size_t i = 0; i < amount - 1; i++)
	{
		separated_pol.clear();
		for (size_t j = 0; j < sep_len; j++)
		{
			separated_pol.push_back(pol[i * sep_len + j]);
		}
		res.push_back(Graham(separated_pol));
	}

	separated_pol.clear();

	for (size_t i = 0; i < sep_len + len % amount; i++)
	{
		separated_pol.push_back(pol[(amount - 1) * sep_len + i]);
	}

	res.push_back(Graham(separated_pol));
	return res;
}

Polygon JarvisInnerBody(vector<Polygon> &chan_result)
{
	size_t chan_len = chan_result.size();
	size_t len1, len2;

	if (chan_result.size() > 1)
	{
		for (size_t i = 0; i < chan_len / 2; i++)
		{
			len1 = chan_result[i].size();
			len2 = chan_result[chan_len / 2 + i].size();

			for (size_t j = 0; j < len2; j++)
			{
				chan_result[i].push_back(chan_result[chan_len / 2 + i][j]);
			}

			chan_result[i] = Graham(chan_result[i]);
			chan_result[chan_len / 2 + i].clear();
			chan_result.erase(chan_result.begin() + chan_len / 2 + i);
			chan_len = chan_result.size();
		}
		JarvisInnerBody(chan_result);
	}
	return chan_result[0];
}

Polygon Jarvis(const Polygon pol, const size_t amount)
{
	vector<Polygon> chan_res = Chan(pol, amount);
	Polygon res = JarvisInnerBody(chan_res);
	return Graham(res);
}

// перемещение точки из from с позиции i в конец to; её копия возвращается в качестве значения
Dot movedot(Polygon &from, const Polygon::iterator it, Polygon &to)
{
	Dot res = *it;
	from.erase(it);
	to.push_back(res);
	return res;
}

Polygon Bruteforce(Polygon pol) // алгоритм построения выпуклого многоугольника перебором
{
	Polygon res;
	if (!pol.size()) return res;

	auto c = pol.begin(); // находим нижнюю левую точку
	for (auto it = pol.begin() + 1; it != pol.end(); ++it)
		if (it->y < c->y || // если очередная точка ниже точки pol[c]
			it->y == c->y && // или если на одном уровне, но левее
			it->x < c->x) c = it; // то выбираем её

	Dot curr = movedot(pol, c, res); // и перемещаем из pol в res, запомнив её
	FlatVec vec1, vec2;

	for (auto i = pol.begin(); i != pol.end();)
	{
		vec1.reFill(curr, *i); // строим направленный отрезок (вектор) из curr в *i
		vec2.reFill(curr, res.front());
		bool flag = isright(vec1, vec2); // чтобы в конце замкнуть многоугольник
		for (auto j = pol.begin(); flag && j != pol.end(); ++j) if (i != j)
		{ // проверяем отрезок из curr в *i
			vec2.reFill(curr, *j); // если найдётся точка, лежащая по другую сторону (1) от отрезка, то
			flag = isright(vec1, vec2); // отрезок из curr в *i нам не подходит
		}
		if (flag)
		{ // если отрезок из curr в *i подошёл, то
			curr = movedot(pol, i, res); // *i перемещается в res, curr обновляется
			i = pol.begin(); // вновь рассматриваем все элементы pol
		}
		// иначе исключаем *i из рассмотрения для текущего значения curr
		else ++i; // и переходим к другой точке
	}
	/*
	(1):
	В данной функции строится многоугольник с направлением обхода против часовой стрелки,
	а значит, необходимо выбирать такие направленные отрезки (векторы), что всякая точка лежит
	в направлении кратчайшего поворота этого вектора против часовой стрелки
	(что и означает, что все они лежат по одну сторону от прямой, проходящей через отрезок).
	Это условие легко проверяется через правоориентированность двух плоских векторов.
	*/
	return res;
}

int main(int argc, char **argv)
{
	cout << "Current directory:\n";
#ifdef _WIN32
	system("cd");
#else
	system("pwd");
#endif
	cout << endl;
	char addr[256], c;
	Polygon pol;

	cout << "Enter <R> or <r> to generate random polygon.\n";
	cout << "Enter any another symbol to read polygon from file.\n";
	cin >> c;
	if (c == 'R' || c == 'r') // создаём случайное множество точек
	{
		size_t len; double low, high;
		cout << "Count of points: "; cin >> len;
		cout << "Low coordinate:  "; cin >> low;
		cout << "High coordinate: "; cin >> high;

		srand((unsigned int)time(0));
		pol = randpol(len, low, high);
		cout << "Save in file: "; cin >> addr;

		ofstream save(addr); // сохраняем его в файл
		save << pol;
		save.close();
	}
	else {
		ifstream fin; // открываем файл для чтения
		do {
			cout << "Input: ";
			cin >> addr;
			fin.open(addr);
			if (!fin.is_open()) cout << "File not found. Please enter some another address.\n";
		}
		while (!fin.is_open());
		fin >> pol; // считываем
		fin.close();
	}

	cout << "Output: "; // открываем файл для записи
	cin >> addr;
	ofstream fout(addr);

	auto t = clock(); // обрабатываем и записываем
	Polygon result = Jarvis(pol, 4);
	auto seconds = (long double)(clock() - t) / CLOCKS_PER_SEC;
	fout << result;

	fout.close(); // завершение работы
	cout << "\nProcess completed in time " << seconds << " sec.\nPress <Enter> to exit.\n";
	cin.get(); cin.get();
	return 0;
}
