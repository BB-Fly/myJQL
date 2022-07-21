#include "myjql.h"
#include <map>
#include <random>
#include <ctime>
#include <vector>

using namespace std;

const int max_size = 1024;

vector<char> get_random_str() {
	vector<char> ans;
	int op = 0;
	char tmp;

	while (op==0 || rand()%1000) {
		op = rand() % 60;
		tmp = 'a' + op - 1;
		ans.push_back(tmp);
		if (ans.size() >= max_size) {
			break;
		}
	}
	return ans;
}

size_t get_num(map<vector<char>, vector<char> >& mp) {
	size_t i = 0;
	for (auto it = mp.begin(); it != mp.end(); it++) {
		i++;
	}
	return i;
}

map<vector<char>, vector<char> >::iterator get_mum(map<vector<char>, vector<char> >& mp) {
	size_t total = get_num(mp);
	size_t idx = rand() % total;

	auto it = mp.begin();

	while (idx--) {
		it++;
	}

	return it;
}

void my_copy(vector<char>& vec, char* str) {
	memset(str, 0, max_size * sizeof(char));
	for (int i = 0; i < vec.size(); i++) {
		str[i] = vec[i];
	}
}

int test(int nums) {
	int op;
	size_t str_size1, str_size2, str_size3;
	vector<char> str1, str2;
	map<vector<char>, vector<char> > mp;
	char str3[max_size], str11[max_size], str22[max_size];

	while (nums--) {

		op = rand() % 5;

		if (op == 0) {
			if (mp.begin() == mp.end()) {
				continue;
			}
			auto it = get_mum(mp);
			str1 = it->first;
			str2 = it->second;
			str_size1 = str1.size();
			str_size2 = str2.size();
			my_copy(str1, str11);
			my_copy(str2, str22);
			memset(str3, 0, max_size * sizeof(char));
			str_size3 = myjql_get(str11, str_size1, str3, max_size);
			if (str_size3 != str_size2) {
				return 0;
			}
			for (size_t i = 0; i < str_size2; i++) {
				if (str2[i] != str3[i]) {
					return 0;
				}
			}

		}
		else if (op == 1) {
			str1 = get_random_str();
			str_size1 = str1.size();
			my_copy(str1, str11);

			memset(str3, 0, max_size * sizeof(char));
			str_size3 = myjql_get(str11, str_size1, str3, max_size);
			if (mp.count(str1) == 0) {
				if (str_size3 == -1) {
					continue;
				}
				else {
					return 0;
				}
			}
			else {
				if (mp.begin() == mp.end()) {
					continue;
				}
				str2 = mp[str1];
				str_size2 = str2.size();
				if (str_size3 != str_size2) {
					return 0;
				}
				for (size_t i = 0; i < str_size2; i++) {
					if (str2[i] != str3[i]) {
						return 0;
					}
				}
			}

		}
		else if (op == 2) {
			if (mp.begin() == mp.end()) {
				continue;
			}
			auto it = get_mum(mp);
			str1 = it->first;
			str2 = it->second;
			str_size1 = str1.size();
			str_size2 = str2.size();
			my_copy(str1, str11);

			mp.erase(it);
			myjql_del(str11, str_size1);

			memset(str3, 0, max_size * sizeof(char));
			str_size3 = myjql_get(str11, str_size1, str3, max_size);
			if (str_size3 != -1) {
				return 0;
			}

		}else
		{
			str1 = get_random_str();
			str_size1 = str1.size();

			str2 = get_random_str();
			str_size2 = str2.size();
			my_copy(str1, str11);
			my_copy(str2, str22);

			mp[str1] = str2;
			myjql_set(str11, str_size1, str22, str_size2);
		}
		printf("now %d left!\n", nums);
	}

	return 1;
}

int main() {

	srand(time(0));


	myjql_init();
	int i = test(100000);
	myjql_close();


	if (i == 0) {
		printf("failed\n");
	}
	else {
		printf("successed\n");
	}
	return 0;

}
