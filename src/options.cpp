// Copyright

void ParseOptions(const std::string &str, allocator *alloc, options *result)
{
	using json = nlohmann::json;

	json j;
	try
	{
		j = json::parse(str);
	}
	catch (std::exception e)
	{
		Println("error reading json options file");
		throw e;
	}

	for (json::iterator it = j.begin(); it != j.end(); ++it)
	{
		option* opt = PushStruct<option>(alloc);
		opt->Type = it.value().type();

		switch (opt->Type)
		{
		case json::value_t::number_float:
			opt->Float = it.value();
			break;

		case json::value_t::number_integer:
			opt->Int = it.value();
			break;

		case json::value_t::number_unsigned:
			opt->Int = it.value();
			break;

		case json::value_t::string:
		{
			std::string value = it.value();
			opt->String = value;
			break;
		}

		case json::value_t::boolean:
			opt->Bool = it.value();
			break;
		}

		//Println(std::string(it.key()));
		//Println(it.value().type_name());
		result->Values[it.key()] = opt;
	}
}