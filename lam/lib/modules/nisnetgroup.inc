<?php

use \LAM\PDF\PDFTable;
use \LAM\PDF\PDFTableCell;
use \LAM\PDF\PDFTableRow;
use LAM\TYPES\ConfiguredType;

/*

  This code is part of LDAP Account Manager (http://www.ldap-account-manager.org/)
  Copyright (C) 2009 - 2024  Roland Gruber

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/**
 * Manages entries based on the object class nisNetgroup.
 *
 * @package modules
 * @author Roland Gruber
 */

/**
 * Manages entries based on the object class nisNetgroup.
 *
 * @package modules
 */
class nisnetgroup extends baseModule {

	/* caches to reduce LDAP queries */
	/** user cache */
	private $cachedUserList;
	/** host cache */
	private $cachedHostList;
	/** group cache */
	private $cachedGroupList;

	/**
	 * Returns true if this module can manage accounts of the current type, otherwise false.
	 *
	 * @return boolean true if module fits
	 */
	public function can_manage() {
		return in_array($this->get_scope(), ['netgroup']);
	}

	/**
	 * Returns meta data that is interpreted by parent class
	 *
	 * @return array array with meta data
	 *
	 * @see baseModule::get_metaData()
	 */
	function get_metaData() {
		$return = [];
		// icon
		$return['icon'] = 'group.svg';
		// alias name
		$return["alias"] = _("NIS net group");
		// this is a base module
		$return["is_base"] = true;
		// LDAP filter
		$return["ldap_filter"] = ['or' => "(objectClass=nisNetgroup)"];
		// RDN attributes
		$return["RDN"] = ["cn" => "normal"];
		// module dependencies
		$return['dependencies'] = ['depends' => [], 'conflicts' => []];
		// managed object classes
		$return['objectClasses'] = ['nisNetgroup'];
		// managed attributes
		$return['attributes'] = ['cn', 'description', 'memberNisNetgroup', 'nisNetgroupTriple'];
		// help Entries
		$return['help'] = [
			'cn' => [
				"Headline" => _("Group name"), 'attr' => 'cn',
				"Text" => _("This is the name of this group.")
			],
			'description' => [
				"Headline" => _("Description"), 'attr' => 'description',
				"Text" => _("Here you can enter a description for this group.")
			],
			'memberNisNetgroup' => [
				"Headline" => _("Subgroups"), 'attr' => 'memberNisNetgroup',
				"Text" => _("Here you can specify subgroups which are included in this NIS netgroup. All members of the subgroups will be treated as members of this group.")
			],
			'members' => [
				"Headline" => _("Members"), 'attr' => 'nisNetgroupTriple',
				"Text" => _("These entries specify the members of the netgroup. You can limit the set to a host name, a user name, a domain name or any combination of them.")
			],
			'membersUpload' => [
				"Headline" => _("Members"), 'attr' => 'nisNetgroupTriple',
				"Text" => _("These entries specify the members of the netgroup. You can limit the set to a host name, a user name, a domain name or any combination of them.") . ' ' .
					_("For the upload please specify the entries in the format \"(HOST,USER,DOMAIN)\". Multiple entries are separated by semicolon.")
			],
			'filter' => [
				"Headline" => _("Filter"),
				"Text" => _("Here you can enter a filter value. Only entries which contain the filter text will be shown.")
			]
		];
		// upload fields
		$return['upload_columns'] = [
			[
				'name' => 'nisnetgroup_cn',
				'description' => _('Group name'),
				'help' => 'cn',
				'example' => _('adminstrators'),
				'required' => true,
				'unique' => true
			],
			[
				'name' => 'nisnetgroup_description',
				'description' => _('Group description'),
				'help' => 'description',
				'example' => _('Administrators group')
			],
			[
				'name' => 'nisnetgroup_subgroups',
				'description' => _('Subgroups'),
				'help' => 'memberNisNetgroup',
				'example' => _('group01,group02')
			],
			[
				'name' => 'nisnetgroup_members',
				'description' => _('Members'),
				'help' => 'membersUpload',
				'example' => _('(host1,user1,example.com);(host2,user2,example.com)')
			]
		];
		// available PDF fields
		$return['PDF_fields'] = [
			'cn' => _('Group name'),
			'description' => _('Description'),
			'subgroups' => _('Subgroups'),
			'members' => _('Members')
		];
		return $return;
	}

	/**
	 * This function fills the $messages variable with output messages from this module.
	 */
	function load_Messages() {
		$this->messages['cn'][0] = ['ERROR', _('Group name'), _('Group name contains invalid characters. Valid characters are: a-z, A-Z, 0-9 and .-_ !')];
		$this->messages['user'][0] = ['ERROR', _('User name'), _('User name contains invalid characters. Valid characters are: a-z, A-Z, 0-9 and .-_ !')];
		$this->messages['host'][0] = ['ERROR', _('Host name'), _('Host name contains invalid characters. Valid characters are: a-z, A-Z, 0-9 and .-_ !')];
		$this->messages['domain'][0] = ['ERROR', _('Domain name'), _('Domain name is invalid!')];
	}

	/**
	 * Returns a list of modifications which have to be made to the LDAP account.
	 *
	 * @return array list of modifications
	 * <br>This function returns an array with 3 entries:
	 * <br>array( DN1 ('add' => array($attr), 'remove' => array($attr), 'modify' => array($attr)), DN2 .... )
	 * <br>DN is the DN to change. It may be possible to change several DNs (e.g. create a new user and add him to some groups via attribute memberUid)
	 * <br>"add" are attributes which have to be added to LDAP entry
	 * <br>"remove" are attributes which have to be removed from LDAP entry
	 * <br>"modify" are attributes which have to been modified in LDAP entry
	 * <br>"info" are values with informational value (e.g. to be used later by pre/postModify actions)
	 */
	function save_attributes() {
		$return = $this->getAccountContainer()->save_module_attributes($this->attributes, $this->orig);
		// nisNetgroupTriple needs special changing
		if (isset($return[$this->getAccountContainer()->dn_orig]['remove']['nisNetgroupTriple'])) {
			$return[$this->getAccountContainer()->dn_orig]['modify']['nisNetgroupTriple'] = $this->attributes['nisNetgroupTriple'];
			unset($return[$this->getAccountContainer()->dn_orig]['remove']['nisNetgroupTriple']);
		}
		if (isset($return[$this->getAccountContainer()->dn_orig]['add']['nisNetgroupTriple'])) {
			$return[$this->getAccountContainer()->dn_orig]['modify']['nisNetgroupTriple'] = $this->attributes['nisNetgroupTriple'];
			unset($return[$this->getAccountContainer()->dn_orig]['add']['nisNetgroupTriple']);
		}
		return $return;
	}

	/**
	 * Returns the HTML meta data for the main account page.
	 *
	 * @return htmlElement HTML meta data
	 */
	function display_html_attributes() {
		$return = new htmlResponsiveRow();
		// user name
		$groupName = '';
		if (isset($this->attributes['cn'][0])) {
			$groupName = $this->attributes['cn'][0];
		}
		$nameInput = new htmlResponsiveInputField(_("Group name"), 'cn', $groupName, 'cn');
		$nameInput->setRequired(true);
		$nameInput->setFieldMaxLength(20);
		$return->add($nameInput, 12);
		$description = '';
		if (isset($this->attributes['description'][0])) {
			$description = $this->attributes['description'][0];
		}
		$return->add(new htmlResponsiveInputField(_('Description'), 'description', $description, 'description'), 12);
		$subgroupsContainer = new htmlTable();
		if (isset($this->attributes['memberNisNetgroup']) && is_array($this->attributes['memberNisNetgroup'])) {
			$membergroups = $this->attributes['memberNisNetgroup'];
			while (sizeof($membergroups) > 0) {
				$parts = array_splice($membergroups, 0, 8);
				$subgroupsContainer->addElement(new htmlOutputText(implode(', ', $parts)), true);
			}
		}
		$subGroupsButtonGroup = new htmlGroup();
		$subGroupsButtonGroup->addElement(new htmlAccountPageButton(get_class($this), 'group', 'open', _('Edit subgroups')));
		$subGroupsButtonGroup->addElement(new htmlHelpLink('memberNisNetgroup'));
		$subgroupsContainer->addElement($subGroupsButtonGroup);
		$subgroupsLabel = new htmlOutputText(_('Subgroups'));
		$return->addLabel($subgroupsLabel);
		$return->addField($subgroupsContainer);
		$return->addLabel(new htmlOutputText('&nbsp;', false));
		// members
		$membersLabel = new htmlSubTitle(_('Members'));
		$membersLabel->setHelpId('members');
		$return->add($membersLabel, 12);
		$titles = [_('Host'), _('User'), _('Domain'), ''];
		$data = [];
		if (isset($this->attributes['nisNetgroupTriple']) && (sizeof($this->attributes['nisNetgroupTriple']) > 0)) {
			for ($i = 0; $i < sizeof($this->attributes['nisNetgroupTriple']); $i++) {
				$triple = substr($this->attributes['nisNetgroupTriple'][$i], 1, strlen($this->attributes['nisNetgroupTriple'][$i]) - 2);
				$triple = explode(',', $triple);
				if (isset($_POST['form_subpage_nisnetgroup_attributes_select']) && ($_POST['type'] == 'host') && ($_POST['position'] == strval($i))) {
					$triple[0] = $_POST['selectBox'];
				}
				if (isset($_POST['form_subpage_nisnetgroup_attributes_select']) && ($_POST['type'] == 'user') && ($_POST['position'] == strval($i))) {
					$triple[1] = $_POST['selectBox'];
				}
				$dataRow = [];
				$hostGroup = new htmlGroup();
				$hostField = new htmlInputField('host_' . $i, $triple[0]);
				$hostField->setCSSClasses(['auto-width']);
				$hostField->setFieldSize(null);
				$hostGroup->addElement($hostField);
				$hostButton = new htmlAccountPageButton(get_class($this), 'select', 'host' . $i, 'computer.svg', true);
				$hostButton->setTitle(_('Select host'));
				$hostGroup->addElement($hostButton);
				$dataRow[] = $hostGroup;
				$userGroup = new htmlGroup();
				$userField = new htmlInputField('user_' . $i, $triple[1]);
				$userField->setCSSClasses(['auto-width']);
				$userField->setFieldSize(null);
				$userGroup->addElement($userField);
				$userButton = new htmlAccountPageButton(get_class($this), 'select', 'user' . $i, 'user.svg', true);
				$userButton->setTitle(_('Select user'));
				$userGroup->addElement($userButton);
				$dataRow[] = $userGroup;
				$domainField = new htmlInputField('domain_' . $i, $triple[2]);
				$domainField->setFieldSize(null);
				$dataRow[] = $domainField;
				$delButton = new htmlButton('del_' . $i, 'del.svg', true);
				$delButton->setTitle(_('Delete'));
				$dataRow[] = $delButton;
				$data[] = $dataRow;
			}
		}
		$dataRowNew = [];
		$hostNew = '';
		$userNew = '';
		$domainNew = '';
		if (isset($_POST['host_new'])) {
			$hostNew = $_POST['host_new'];
		}
		if (isset($_POST['user_new'])) {
			$userNew = $_POST['user_new'];
		}
		if (isset($_POST['domain_new'])) {
			$domainNew = $_POST['domain_new'];
		}
		$hostNewGroup = new htmlGroup();
		$hostField = new htmlInputField('host_new', $hostNew);
		$hostField->setCSSClasses(['auto-width']);
		$hostField->setFieldSize(null);
		$hostNewGroup->addElement($hostField);
		$hostButton = new htmlAccountPageButton(get_class($this), 'select', 'hostNew', 'computer.svg', true);
		$hostButton->setTitle(_('Select host'));
		$hostNewGroup->addElement($hostButton);
		$dataRowNew[] = $hostNewGroup;
		$userNewGroup = new htmlGroup();
		$userField = new htmlInputField('user_new', $userNew);
		$userField->setCSSClasses(['auto-width']);
		$userField->setFieldSize(null);
		$userNewGroup->addElement($userField);
		$userButton = new htmlAccountPageButton(get_class($this), 'select', 'userNew', 'user.svg', true);
		$userButton->setTitle(_('Select user'));
		$userNewGroup->addElement($userButton);
		$dataRowNew[] = $userNewGroup;
		$domainField = new htmlInputField('domain_new', $domainNew);
		$domainField->setFieldSize(null);
		$dataRowNew[] = $domainField;
		$addButton = new htmlButton('add_new', 'add.svg', true);
		$addButton->setTitle(_('Add'));
		$dataRowNew[] = $addButton;
		$data[] = $dataRowNew;

		$memberTable = new htmlResponsiveTable($titles, $data);
		$return->add($memberTable, 12);
		return $return;
	}

	/**
	 * Processes user input of the primary module page.
	 * It checks if all input values are correct and updates the associated LDAP attributes.
	 *
	 * @return array list of info/error messages
	 */
	function process_attributes() {
		$errors = [];
		// user name
		$this->attributes['cn'][0] = $_POST['cn'];
		if (!get_preg($this->attributes['cn'][0], 'groupname')) {
			$errors[] = $this->messages['cn'][0];
		}
		// description
		$this->attributes['description'][0] = $_POST['description'];
		// members
		$this->attributes['nisNetgroupTriple'] = [];
		$i = 0;
		while (isset($_POST['host_' . $i]) || isset($_POST['user_' . $i]) || isset($_POST['domain_' . $i])) {
			if (isset($_POST['del_' . $i])) {
				$i++;
				continue;
			}
			// build NIS triple
			$this->attributes['nisNetgroupTriple'][] = '(' . $_POST['host_' . $i] . ',' . $_POST['user_' . $i] . ',' . $_POST['domain_' . $i] . ')';
			// check user input
			if (($_POST['host_' . $i] != '') && !get_preg($_POST['host_' . $i], 'DNSname')) {
				$message = $this->messages['host'][0];
				$message[2] = $message[2] . '<br><br>' . $_POST['host_' . $i];
				$errors[] = $message;
			}
			if (($_POST['user_' . $i] != '') && !get_preg($_POST['user_' . $i], 'username')) {
				$message = $this->messages['user'][0];
				$message[2] = $message[2] . '<br><br>' . $_POST['user_' . $i];
				$errors[] = $message;
			}
			if (($_POST['domain_' . $i] != '') && !get_preg($_POST['domain_' . $i], 'DNSname')) {
				$message = $this->messages['domain'][0];
				$message[2] = $message[2] . '<br><br>' . $_POST['domain_' . $i];
				$errors[] = $message;
			}
			$i++;
		}
		$addNewTriple = isset($_POST['add_new']);
		// check user input
		if (($_POST['host_new'] != '') && !get_preg($_POST['host_new'], 'DNSname')) {
			$message = $this->messages['host'][0];
			$message[2] = $message[2] . '<br><br>' . $_POST['host_new'];
			$errors[] = $message;
			$addNewTriple = false;
		}
		if (($_POST['user_new'] != '') && !get_preg($_POST['user_new'], 'username')) {
			$message = $this->messages['user'][0];
			$message[2] = $message[2] . '<br><br>' . $_POST['user_new'];
			$errors[] = $message;
			$addNewTriple = false;
		}
		if (($_POST['domain_new'] != '') && !get_preg($_POST['domain_new'], 'DNSname')) {
			$message = $this->messages['domain'][0];
			$message[2] = $message[2] . '<br><br>' . $_POST['domain_new'];
			$errors[] = $message;
			$addNewTriple = false;
		}
		if ($addNewTriple) {
			$this->attributes['nisNetgroupTriple'][] = '(' . $_POST['host_new'] . ',' . $_POST['user_new'] . ',' . $_POST['domain_new'] . ')';
			unset($_POST['host_new']);
			unset($_POST['user_new']);
			unset($_POST['domain_new']);
		}
		return $errors;
	}

	/**
	 * Displays the group selection.
	 *
	 * @return htmlElement meta HTML code
	 */
	function display_html_group() {
		// load list with all groups
		$allGroups = $this->getGroupList();
		// remove own entry
		if (!$this->getAccountContainer()->isNewAccount) {
			$allGroups = array_delete($this->attributes['cn'][0], $allGroups);
		}
		$subgroups = [];
		if (is_array($this->attributes['memberNisNetgroup'])) {
			$subgroups = $this->attributes['memberNisNetgroup'];
			$allGroups = array_delete($subgroups, $allGroups);
		}
		$selectedGroups = [];
		foreach ($subgroups as $subgroup) {
			$selectedGroups[$subgroup] = $subgroup;
		}
		$availableGroups = [];
		foreach ($allGroups as $availableGroup) {
			$availableGroups[$availableGroup] = $availableGroup;
		}

		$return = new htmlResponsiveRow();
		$return->add(new htmlSubTitle(_("Subgroups")), 12);

		$this->addDoubleSelectionArea($return, _("Selected groups"), _("Available groups"), $selectedGroups, [], $availableGroups, [], 'subgroup', false, true);

		$return->addVerticalSpacer('2rem');
		$backButton = new htmlAccountPageButton(get_class($this), 'attributes', 'back', _('Back'));
		$return->add($backButton, 12);
		return $return;
	}

	/**
	 * Processes user input of the group selection page.
	 * It checks if all input values are correct and updates the associated LDAP attributes.
	 *
	 * @return array list of info/error messages
	 */
	function process_group() {
		if (isset($_POST['subgroup_2']) && isset($_POST['subgroup_left'])) { // Add groups to list
			if (!is_array($this->attributes['memberNisNetgroup'])) {
				$this->attributes['memberNisNetgroup'] = [];
			}
			// Add new group
			$this->attributes['memberNisNetgroup'] = @array_merge($this->attributes['memberNisNetgroup'], $_POST['subgroup_2']);
		}
		elseif (isset($_POST['subgroup_1']) && isset($_POST['subgroup_right'])) { // remove groups from list
			$this->attributes['memberNisNetgroup'] = array_delete($_POST['subgroup_1'], $this->attributes['memberNisNetgroup']);
		}
		return [];
	}

	/**
	 * Displays the host/user selection.
	 *
	 * @return htmlElement meta HTML code
	 */
	function display_html_select() {
		$return = new htmlResponsiveRow();
		$selectHost = true;
		$postKeys = array_keys($_POST);
		$position = 'New';
		for ($i = 0; $i < sizeof($postKeys); $i++) {
			if (substr($postKeys[$i], 0, 36) == 'form_subpage_nisnetgroup_select_user') {
				$selectHost = false;
				$position = substr($postKeys[$i], 36);
				break;
			}
			if (substr($postKeys[$i], 0, 36) == 'form_subpage_nisnetgroup_select_host') {
				$position = substr($postKeys[$i], 36);
				break;
			}
		}
		if ($selectHost) {
			// load list with all hosts
			$options = $this->getHostList();
			$count = sizeof($options);
			for ($i = 0; $i < $count; $i++) {
				if (!get_preg($options[$i], 'DNSname')) {
					unset($options[$i]);
				}
			}
			$options = array_values($options);
		}
		else {
			// load list with all users
			$options = $this->getUserList();
			$options = array_values($options);
		}
		$return->addLabel(new htmlOutputText(_('Filter')));
		$filterInput = new htmlInputField('filter');
		$filterInput->filterSelectBox('selectBox');
		$return->addField($filterInput);
		$title = _('Host name');
		if (!$selectHost) {
			$title = _('User name');
		}
		$return->addLabel(new htmlOutputText($title));
		$return->addField(new htmlSelect('selectBox', $options));
		$return->addVerticalSpacer('2rem');
		$type = 'host';
		if (!$selectHost) {
			$type = 'user';
		}
		$return->addLabel(new htmlAccountPageButton(get_class($this), 'attributes', 'select', _('Ok')));
		$return->addField(new htmlAccountPageButton(get_class($this), 'attributes', 'back', _('Cancel')));
		$return->add(new htmlHiddenInput('host_new', $_POST['host_new']), 12);
		$return->add(new htmlHiddenInput('user_new', $_POST['user_new']), 12);
		$return->add(new htmlHiddenInput('domain_new', $_POST['domain_new']), 12);
		$return->add(new htmlHiddenInput('type', $type), 12);
		$return->add(new htmlHiddenInput('position', $position), 12);
		return $return;
	}

	/**
	 * Processes user input of the host/user selection page.
	 * It checks if all input values are correct and updates the associated LDAP attributes.
	 *
	 * @return array list of info/error messages
	 */
	function process_select() {
		if (isset($_POST['form_subpage_nisnetgroup_attributes_back'])) {
			return [];
		}
		if ($_POST['position'] == 'New') {
			$_POST[$_POST['type'] . '_new'] = $_POST['selectBox'];
		}
		return [];
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::build_uploadAccounts()
	 */
	function build_uploadAccounts($rawAccounts, $ids, &$partialAccounts, $selectedModules, &$type) {
		$messages = [];
		// get list of existing groups
		$existingGroups = $this->getGroupList();
		for ($i = 0; $i < sizeof($rawAccounts); $i++) {
			// add object class
			if (!in_array('nisNetgroup', $partialAccounts[$i]['objectClass'])) {
				$partialAccounts[$i]['objectClass'][] = 'nisNetgroup';
			}
			// add cn
			$partialAccounts[$i]['cn'] = $rawAccounts[$i][$ids['nisnetgroup_cn']];
			// description (UTF-8, no regex check needed)
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'nisnetgroup_description', 'description');
			// additional groups
			if ($rawAccounts[$i][$ids['nisnetgroup_subgroups']] != "") {
				$groups = explode(",", $rawAccounts[$i][$ids['nisnetgroup_subgroups']]);
				$skipSubgroups = false;
				for ($g = 0; $g < sizeof($groups); $g++) {
					if (!in_array($groups[$g], $existingGroups)) {
						$messages[] = ['ERROR', _('Unable to find group in LDAP.'), $groups[$g]];
						$skipSubgroups = true;
					}
				}
				if (!$skipSubgroups) {
					$partialAccounts[$i]['memberNisNetgroup'] = $groups;
				}
			}
			// members
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'nisnetgroup_members', 'nisNetgroupTriple', null, null, $messages, '/;[ ]*/');
		}
		return $messages;
	}

	/**
	 * This functions is used to check if all settings for this module have been made.
	 *
	 * @return boolean true, if settings are complete
	 */
	function module_complete() {
		return (isset($this->attributes['cn']) && (sizeof($this->attributes['cn']) > 0));
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::get_pdfEntries()
	 */
	function get_pdfEntries($pdfKeys, $typeId) {
		$return = [];
		$this->addSimplePDFField($return, 'cn', _('Group name'));
		$this->addSimplePDFField($return, 'description', _('Description'));
		$this->addSimplePDFField($return, 'subgroups', _('Subgroups'), 'memberNisNetgroup');
		if (sizeof($this->attributes['nisNetgroupTriple']) > 0) {
			$pdfTable = new PDFTable(_('Members'));
			$pdfRow = new PDFTableRow();
			$pdfRow->cells[] = new PDFTableCell(_('Host'), '20%', null, true);
			$pdfRow->cells[] = new PDFTableCell(_('User'), '20%', null, true);
			$pdfRow->cells[] = new PDFTableCell(_('Domain'), '20%', null, true);
			$pdfTable->rows[] = $pdfRow;
			for ($i = 0; $i < sizeof($this->attributes['nisNetgroupTriple']); $i++) {
				$triple = substr($this->attributes['nisNetgroupTriple'][$i], 1, strlen($this->attributes['nisNetgroupTriple'][$i]) - 2);
				$triple = explode(',', $triple);
				$pdfRow = new PDFTableRow();
				$pdfRow->cells[] = new PDFTableCell($triple[0], '20%');
				$pdfRow->cells[] = new PDFTableCell($triple[1], '20%');
				$pdfRow->cells[] = new PDFTableCell($triple[2], '30%');
				$pdfTable->rows[] = $pdfRow;
			}
			$this->addPDFTable($return, 'members', $pdfTable);
		}
		return $return;
	}

	/**
	 * Returns a list of existing NIS net groups.
	 *
	 * @return array group names
	 */
	private function getGroupList() {
		if ($this->cachedGroupList != null) {
			return $this->cachedGroupList;
		}
		$this->cachedGroupList = searchLDAPByAttribute('cn', '*', 'nisNetgroup', ['cn'], ['netgroup']);
		for ($i = 0; $i < sizeof($this->cachedGroupList); $i++) {
			$this->cachedGroupList[$i] = $this->cachedGroupList[$i]['cn'][0];
		}
		return $this->cachedGroupList;
	}

	/**
	 * Returns a list of existing users.
	 *
	 * @return array user names
	 */
	private function getUserList() {
		if ($this->cachedUserList != null) {
			return $this->cachedUserList;
		}
		$this->cachedUserList = searchLDAPByAttribute('uid', '*', 'posixAccount', ['uid'], ['user']);
		for ($i = 0; $i < sizeof($this->cachedUserList); $i++) {
			$this->cachedUserList[$i] = $this->cachedUserList[$i]['uid'][0];
		}
		return $this->cachedUserList;
	}

	/**
	 * Returns a list of existing hosts.
	 *
	 * @return array host names
	 */
	private function getHostList() {
		if ($this->cachedHostList != null) {
			return $this->cachedHostList;
		}
		$this->cachedHostList = [];
		$list = searchLDAPByFilter('(|(objectClass=account)(objectClass=device))', ['uid', 'cn'], ['host']);
		foreach ($list as $attrs) {
			if (!empty($attrs['uid'][0])) {
				$this->cachedHostList[] = $attrs['uid'][0];
			}
			if (!empty($attrs['cn'][0])) {
				$this->cachedHostList[] = $attrs['cn'][0];
			}
		}
		$this->cachedHostList = array_values(array_unique($this->cachedHostList));
		return $this->cachedHostList;
	}

	/**
	 * @inheritDoc
	 */
	public function getListAttributeDescriptions(ConfiguredType $type): array {
		return [
			"cn" => _("Group name"),
			'description' => _('Description'),
			'membernisnetgroup' => _('Subgroups'),
			'nisnetgrouptriple' => _('Members')
		];
	}

}
