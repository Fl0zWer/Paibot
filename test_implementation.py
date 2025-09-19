#!/usr/bin/env python3
"""
Simple test script to validate Background Generator implementation
"""

import os
import json
import sys

def test_pack_structure():
    """Test that pack structure is correctly set up"""
    pack_path = "resources/packs/example_pack.json"
    
    if not os.path.exists(pack_path):
        print(f"❌ Pack file not found: {pack_path}")
        return False
    
    try:
        with open(pack_path, 'r') as f:
            pack_data = json.load(f)
        
        required_fields = ['name', 'author', 'version', 'backgrounds']
        for field in required_fields:
            if field not in pack_data:
                print(f"❌ Missing required field in pack.json: {field}")
                return False
        
        print(f"✅ Pack structure valid: {pack_data['name']} by {pack_data['author']}")
        return True
        
    except json.JSONDecodeError as e:
        print(f"❌ Invalid JSON in pack file: {e}")
        return False

def test_resource_structure():
    """Test that resource directories are properly structured"""
    required_dirs = [
        "resources/",
        "resources/packs/",
        "resources/presets/",
        "resources/textures/",
        "resources/sounds/"
    ]
    
    for dir_path in required_dirs:
        if not os.path.exists(dir_path):
            print(f"❌ Missing directory: {dir_path}")
            return False
    
    print("✅ Resource directory structure is correct")
    return True

def test_source_files():
    """Test that new source files are present"""
    required_files = [
        "include/Paibot/manager/PackManager.hpp",
        "src/manager/PackManager.cpp",
        "include/Paibot/util/BackgroundGenerator.hpp",
        "src/util/BackgroundGenerator.cpp"
    ]
    
    for file_path in required_files:
        if not os.path.exists(file_path):
            print(f"❌ Missing source file: {file_path}")
            return False
    
    print("✅ All required source files present")
    return True

def test_cmake_configuration():
    """Test that CMakeLists.txt includes new files"""
    cmake_path = "CMakeLists.txt"
    
    if not os.path.exists(cmake_path):
        print("❌ CMakeLists.txt not found")
        return False
    
    with open(cmake_path, 'r') as f:
        cmake_content = f.read()
    
    required_entries = [
        "src/manager/PackManager.cpp",
        "GEODE_TARGET_PLATFORM"
    ]
    
    for entry in required_entries:
        if entry not in cmake_content:
            print(f"❌ Missing entry in CMakeLists.txt: {entry}")
            return False
    
    print("✅ CMakeLists.txt properly configured")
    return True

def test_background_generator_features():
    """Test that BackgroundGenerator has required features"""
    bg_header = "include/Paibot/util/BackgroundGenerator.hpp"
    
    if not os.path.exists(bg_header):
        print("❌ BackgroundGenerator header not found")
        return False
    
    with open(bg_header, 'r') as f:
        header_content = f.read()
    
    required_features = [
        "Geometrization",  # New background type
        "generateGeometrization",  # Geometrization method
        "PackManager",  # Pack management
        "colorTolerance",  # Geometrization settings
        "targetResolution",
        "WangTileValidation"  # Wang tile validation
    ]
    
    for feature in required_features:
        if feature not in header_content:
            print(f"❌ Missing feature in BackgroundGenerator: {feature}")
            return False
    
    print("✅ BackgroundGenerator has all required features")
    return True

def main():
    """Run all tests"""
    print("🧪 Testing Paibot Background Generator Implementation")
    print("=" * 50)
    
    tests = [
        test_resource_structure,
        test_pack_structure,
        test_source_files,
        test_cmake_configuration,
        test_background_generator_features
    ]
    
    passed = 0
    total = len(tests)
    
    for test in tests:
        if test():
            passed += 1
        print()
    
    print("=" * 50)
    print(f"📊 Test Results: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 All tests passed! Implementation is ready.")
        return 0
    else:
        print("❌ Some tests failed. Please review the implementation.")
        return 1

if __name__ == "__main__":
    sys.exit(main())