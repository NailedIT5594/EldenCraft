import json

def load_json_file(file_path):
    with open(file_path, 'r') as file:
        return json.load(file)

def check_weapons_sheet(weapons_sheet):
    for weapon in weapons_sheet['weapons']:
        if not weapon.get('id'):
            raise ValueError(f"Missing 'id' for weapon: {weapon['item_name']}")
        if not weapon.get('item_name'):
            raise ValueError(f"Missing 'item_name' for weapon: {weapon['id']}")
        if not weapon.get('minecraft_counterpart'):
            raise ValueError(f"Missing 'minecraft_counterpart' for weapon: {weapon['id']}")
        if not weapon.get('base_damage'):
            raise ValueError(f"Missing 'base_damage' for weapon: {weapon['id']}")
        if not weapon.get('attack_speed'):
            raise ValueError(f"Missing 'attack_speed' for weapon: {weapon['id']}")
        if not weapon.get('crit_multiplier'):
            raise ValueError(f"Missing 'crit_multiplier' for weapon: {weapon['id']}")

def check_movement_sheet(movement_sheet):
    for param in movement_sheet['movement_parameters']:
        if not param.get('parameter_name'):
            raise ValueError(f"Missing 'parameter_name' for movement parameter: {param['parameter_name']}")
        if not param.get('minecraft_target_value'):
            raise ValueError(f"Missing 'minecraft_target_value' for movement parameter: {param['parameter_name']}")
        if not param.get('elden_ring_default'):
            raise ValueError(f"Missing 'elden_ring_default' for movement parameter: {param['parameter_name']}")
        if not param.get('status'):
            raise ValueError(f"Missing 'status' for movement parameter: {param['parameter_name']}")

def main():
    weapons_sheet = load_json_file('weapons_sheet.json')
    movement_sheet = load_json_file('movement_sheet.json')

    check_weapons_sheet(weapons_sheet)
    check_movement_sheet(movement_sheet)

    print("All checks passed. Ready to package for Melty.")

if __name__ == '__main__':
    main()
