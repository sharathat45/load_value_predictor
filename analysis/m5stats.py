from typing import Any, Dict, Union
import re

DUMP_BEGIN_HEADER = "---------- Begin Simulation Statistics ----------"
DUMP_END_HEADER = "---------- End Simulation Statistics   ----------"

P_STAT_CATEGORY_SEP = re.compile(r"(?:\.|::)")
P_STAT_CATEGORY = re.compile(r"^(?P<cat>\w+)(?P<sep>\.|::)(?P<rest>.*)$")


class Stat:
    def __init__(self, name: str, value: Union[int, float], desc: str):
        self.name = name
        self.value = value
        self.desc = desc


class StatCategory:
    def __init__(self, name: str):
        self.stats: Dict[str, Union[Stat, StatCategory]] = {}

    def add_from_line(self, stat_line: str):
        m = re.match(P_STAT_CATEGORY, stat_line)

        # If the line does not have a category, instantiate a Stat object
        if m is None:
            name, value, rest = stat_line.split(maxsplit=2)
            extra, desc = rest.split(sep="#", maxsplit=1)

            try:
                value = int(value)
            except ValueError:
                value = float(value)

            self.stats[name] = Stat(name, value, desc)

            return

        # If the line does have a category, extract the captured parts
        cat, sep, rest = m.groups()

        # Check if a category with this name already exists
        if cat in self.stats:
            # If it does, add the rest of this line to that category
            self.stats[cat].add_from_line(rest)
            return

        # If it doesn't already exist, create the category and add this line
        self.stats[cat] = StatCategory(cat)
        self.stats[cat].add_from_line(rest)

    def as_dict(self) -> Dict[str, Union[int, float]]:
        result = {}

        for stat in self.stats.values():
            if isinstance(stat, StatCategory):
                interm = stat.as_dict()
                for k, v in interm:
                    result[f"{stat.name}.{k}"] = v
            else:
                result[stat.name] = stat.value

        return result

    def __getitem__(self, item: str) -> Union[Stat, "StatCategory"]:
        return self.stats[item]


class M5Stats:
    def __init__(self, filepath: str, dumpnum: int = 1):
        with open(filepath, "r") as stats_file:
            # Iterate until we find the start of our desired dump
            for dump in range(dumpnum):
                for line in stats_file:
                    if line.strip() == DUMP_BEGIN_HEADER:
                        break
                else:
                    raise RuntimeError(f"Dump {dump} not found in {filepath}")

            self.stats = StatCategory("all")

            # Iterate through the dump lines and construct stat objects
            for line in stats_file:
                line = line.strip()

                if not line:
                    continue

                if line == DUMP_END_HEADER:
                    break

                try:
                    self.stats.add_from_line(line)
                except Exception as exc:
                    raise RuntimeError(f"Error parsing '{line}'") from exc

    def get_stats(self, path: str = "") -> StatCategory:
        if not path:
            return self.stats

        categories = re.split(P_STAT_CATEGORY_SEP, path)
        current = self.stats
        for category in categories:
            current = current.stats[category]

        return current
