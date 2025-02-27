<?php
/*

  This code is part of LDAP Account Manager (http://www.ldap-account-manager.org/)
  Copyright (C) 2013 - 2024  Roland Gruber

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
 * Manages Windows AD (e.g. Samba 4) hosts.
 *
 * @package modules
 * @author Roland Gruber
 */

use LAM\TYPES\ConfiguredType;
use LAM\TYPES\TypeManager;

/**
 * Manages Windows AD (e.g. Samba 4) hosts.
 *
 * @package modules
 */
class windowsHost extends baseModule {

	/**
	 * These attributes will be ignored by default if a new account is copied from an existing one.
	 */
	private const ATTRIBUTES_TO_IGNORE_ON_COPY = ['sAMAccountName', 'pwdLastSet', 'lastLogonTimestamp',
		'logonCount'];

	/**
	 * Returns true if this module can manage accounts of the current type, otherwise false.
	 *
	 * @return boolean true if module fits
	 */
	public function can_manage() {
		return in_array($this->get_scope(), ['host']);
	}

	/**
	 * Returns meta data that is interpreted by parent class
	 *
	 * @return array array with meta data
	 *
	 * @see baseModule::get_metaData()
	 */
	public function get_metaData() {
		$return = [];
		// icon
		$return['icon'] = 'samba.svg';
		// this is a base module
		$return["is_base"] = true;
		// RDN attribute
		$return["RDN"] = ["cn" => "high"];
		// LDAP filter
		$return["ldap_filter"] = ['and' => "", 'or' => '(objectClass=computer)'];
		// alias name
		$return["alias"] = _("Windows");
		// module dependencies
		$return['dependencies'] = ['depends' => [], 'conflicts' => []];
		// managed object classes
		$return['objectClasses'] = ['computer', 'securityPrincipal'];
		// managed attributes
		$return['attributes'] = ['cn', 'description', 'location', 'sAMAccountName', 'managedBy',
			'operatingSystem', 'operatingSystemVersion', 'dNSHostName', 'pwdLastSet', 'lastLogonTimestamp',
			'logonCount'];
		// help Entries
		$return['help'] = [
			'cn' => [
				"Headline" => _('Host name'), 'attr' => 'cn, sAMAccountName',
				"Text" => _('Please enter the host name.')
			],
			'description' => [
				"Headline" => _('Description'), 'attr' => 'description',
				"Text" => _('Please enter a descriptive text for this host.')
			],
			'location' => [
				"Headline" => _('Location'), 'attr' => 'location',
				"Text" => _('This is the host\'s location (e.g. Munich, server room 3).')
			],
			'managedBy' => [
				"Headline" => _('Managed by'), 'attr' => 'managedBy',
				"Text" => _('The host is managed by this contact person.')
			],
			'pwdLastSet' => [
				"Headline" => _('Last password change'), 'attr' => 'pwdLastSet',
				"Text" => _('Time of user\'s last password change.')
			],
			'lastLogonTimestamp' => [
				"Headline" => _('Last login'), 'attr' => 'lastLogonTimestamp',
				"Text" => _('Time of user\'s last login.')
			],
			'logonCount' => [
				"Headline" => _('Logon count'), 'attr' => 'logonCount',
				"Text" => _('This is the number of logins performed by this account.')
			],
		];
		// upload fields
		$return['upload_columns'] = [
			[
				'name' => 'windowsHost_name',
				'description' => _('Host name'),
				'help' => 'cn',
				'example' => _('PC01'),
				'required' => true
			],
			[
				'name' => 'windowsHost_description',
				'description' => _('Description'),
				'help' => 'description',
			],
			[
				'name' => 'windowsHost_location',
				'description' => _('Location'),
				'help' => 'location',
				'example' => _('MyCity'),
			],
			[
				'name' => 'windowsHost_managedBy',
				'description' => _('Managed by'),
				'help' => 'managedBy',
				'example' => 'cn=user1,o=test',
			],
		];
		// available PDF fields
		$return['PDF_fields'] = [
			'cn' => _('Host name'),
			'description' => _('Description'),
			'location' => _('Location'),
			'managedBy' => _('Managed by'),
		];
		return $return;
	}

	/**
	 * This function fills the $messages variable with output messages from this module.
	 */
	public function load_Messages() {
		$this->messages['cn'][0] = ['ERROR', _('Host name'), _('Host name contains invalid characters. Valid characters are: a-z, A-Z, 0-9 and .-_ !')];
		$this->messages['cn'][1] = ['ERROR', _('Account %s:') . ' windowsHost_cn', _('Host name contains invalid characters. Valid characters are: a-z, A-Z, 0-9 and .-_ !')];
	}

	/**
	 * {@inheritDoc}
	 */
	public function loadAttributesFromAccountCopy(array $ldapAttributes, array $attributesToIgnore = []): void {
		$attributesToIgnore = array_merge(baseModule::ATTRIBUTES_TO_IGNORE_ON_COPY_DEFAULT, self::ATTRIBUTES_TO_IGNORE_ON_COPY);
		parent::loadAttributesFromAccountCopy($ldapAttributes, $attributesToIgnore);
	}

	/**
	 * Returns the HTML meta data for the main account page.
	 *
	 * @return htmlElement HTML meta data
	 */
	public function display_html_attributes() {
		$container = new htmlResponsiveRow();
		$this->addSimpleInputTextField($container, 'cn', _('Host name'), true);
		$this->addSimpleInputTextField($container, 'description', _('Description'), false);
		$this->addSimpleInputTextField($container, 'location', _('Location'), false);
		// last password change
		if (!empty($this->attributes['pwdLastSet'])) {
			$container->addLabel(new htmlOutputText(_('Last password change')));
			$pwdLastSetGroup = new htmlGroup();
			$pwdLastSetGroup->addElement(new htmlOutputText($this->formatFileTime($this->attributes['pwdLastSet'][0])));
			$pwdLastSetGroup->addElement(new htmlSpacer('0.5rem', null));
			$pwdLastSetGroup->addElement(new htmlHelpLink('pwdLastSet'));
			$container->addField($pwdLastSetGroup);
		}
		// last login
		if (!empty($this->attributes['lastLogonTimestamp'])) {
			$container->addLabel(new htmlOutputText(_('Last login')));
			$lastLogonTimestampGroup = new htmlGroup();
			$lastLogonTimestampGroup->addElement(new htmlOutputText($this->formatFileTime($this->attributes['lastLogonTimestamp'][0])));
			$lastLogonTimestampGroup->addElement(new htmlSpacer('0.5rem', null));
			$lastLogonTimestampGroup->addElement(new htmlHelpLink('lastLogonTimestamp'));
			$container->addField($lastLogonTimestampGroup);
		}
		// logon count
		if (!empty($this->attributes['logonCount'])) {
			$container->addLabel(new htmlOutputText(_('Logon count')));
			$logonCountGroup = new htmlGroup();
			$logonCountGroup->addElement(new htmlOutputText($this->attributes['logonCount'][0]));
			$logonCountGroup->addElement(new htmlSpacer('0.5rem', null));
			$logonCountGroup->addElement(new htmlHelpLink('logonCount'));
			$container->addField($logonCountGroup);
		}
		// managed by
		$container->addLabel(new htmlOutputText(_('Managed by')));
		$managedBy = '-';
		if (isset($this->attributes['managedBy'][0])) {
			$managedBy = $this->attributes['managedBy'][0];
		}
		$managedByGroup = new htmlGroup();
		$managedByGroup->addElement(new htmlOutputText(getAbstractDN($managedBy)));
		$managedByGroup->addElement(new htmlSpacer('0.5rem', null));
		$managedByGroup->addElement(new htmlHelpLink('managedBy'), true);
		$container->addField($managedByGroup);
		$container->addLabel(new htmlOutputText('&nbsp;', false));
		$managedByButtons = new htmlGroup();
		$managedByButtons->addElement(new htmlAccountPageButton(get_class($this), 'managedBy', 'edit', _('Change')));
		if (isset($this->attributes['managedBy'][0])) {
			$managedByButtons->addElement(new htmlSpacer('5px', null));
			$managedByButtons->addElement(new htmlAccountPageButton(get_class($this), 'attributes', 'removeManagedBy', _('Remove')));
		}
		$container->addField($managedByButtons);
		return $container;
	}

	/**
	 * Processes user input of the primary module page.
	 * It checks if all input values are correct and updates the associated LDAP attributes.
	 *
	 * @return array list of info/error messages
	 */
	public function process_attributes() {
		$return = [];
		// cn
		$this->attributes['cn'][0] = $_POST['cn'];
		$this->attributes['sAMAccountName'][0] = $_POST['cn'] . '$';
		if (!get_preg($_POST['cn'], 'hostname')) {
			$return[] = $this->messages['cn'][0];
		}
		// description
		$this->attributes['description'][0] = $_POST['description'];
		// location
		$this->attributes['location'][0] = $_POST['location'];
		// managed by
		if (isset($_POST['form_subpage_' . get_class($this) . '_attributes_removeManagedBy'])) {
			unset($this->attributes['managedBy']);
		}
		if ($this->getAccountContainer()->isNewAccount) {
			$this->attributes['userAccountControl'][0] = 4128; // machine trust account, no password required
		}
		return $return;
	}

	/**
	 * This function will create the meta HTML code to show a page to change the member attribute.
	 *
	 * @return htmlElement HTML meta data
	 */
	function display_html_managedBy() {
		$return = new htmlResponsiveRow();
		// show possible managers
		$options = [];
		$filter = get_ldap_filter('user');
		$entries = searchLDAPByFilter($filter, ['dn'], ['user']);
		for ($i = 0; $i < sizeof($entries); $i++) {
			$entries[$i] = $entries[$i]['dn'];
		}
		// sort by DN
		usort($entries, 'compareDN');
		for ($i = 0; $i < sizeof($entries); $i++) {
			$options[getAbstractDN($entries[$i])] = $entries[$i];
		}
		$selected = [];
		if (isset($this->attributes['managedBy'][0])) {
			$selected = [$this->attributes['managedBy'][0]];
			if (!in_array($selected[0], $options)) {
				$options[getAbstractDN($selected[0])] = $selected[0];
			}
		}
		$membersSelect = new htmlSelect('managedBy', $options, $selected);
		$membersSelect->setHasDescriptiveElements(true);
		$membersSelect->setRightToLeftTextDirection(true);
		$membersSelect->setSortElements(false);
		$membersSelect->setTransformSingleSelect(false);
		$return->add($membersSelect, 12);
		$return->addVerticalSpacer('2rem');
		$buttonTable = new htmlTable();
		$buttonTable->addElement(new htmlAccountPageButton(get_class($this), 'attributes', 'set', _('Change')));
		$buttonTable->addElement(new htmlAccountPageButton(get_class($this), 'attributes', 'cancel', _('Cancel')));
		$return->add($buttonTable, 12);
		return $return;
	}

	/**
	 * Processes user input of the members page.
	 * It checks if all input values are correct and updates the associated LDAP attributes.
	 *
	 * @return array list of info/error messages
	 */
	function process_managedBy() {
		$return = [];
		if (isset($_POST['form_subpage_' . get_class($this) . '_attributes_set'])) {
			$this->attributes['managedBy'][0] = $_POST['managedBy'];
		}
		return $return;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::build_uploadAccounts()
	 */
	public function build_uploadAccounts($rawAccounts, $ids, &$partialAccounts, $selectedModules, &$type) {
		$errors = [];
		for ($i = 0; $i < sizeof($rawAccounts); $i++) {
			// add object class
			if (!in_array('computer', $partialAccounts[$i]['objectClass'])) {
				$partialAccounts[$i]['objectClass'][] = 'computer';
			}
			// cn + sAMAccountName
			if ($rawAccounts[$i][$ids['windowsHost_name']] != "") {
				if (get_preg($rawAccounts[$i][$ids['windowsHost_name']], 'hostname')) {
					$partialAccounts[$i]['cn'] = $rawAccounts[$i][$ids['windowsHost_name']];
					$partialAccounts[$i]['sAMAccountName'] = $rawAccounts[$i][$ids['windowsHost_name']] . '$';
				}
				else {
					$errMsg = $this->messages['cn'][1];
					array_push($errMsg, [$i]);
					$errors[] = $errMsg;
				}
			}
			// description
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'windowsHost_description', 'description');
			// location
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'windowsHost_location', 'location');
			// managed by
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'windowsHost_managedBy', 'managedBy');
			// machine trust account, no password required
			$partialAccounts[$i]['userAccountControl'][0] = 4128;
		}
		return $errors;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::get_pdfEntries()
	 */
	public function get_pdfEntries($pdfKeys, $typeId) {
		$return = [];
		$this->addSimplePDFField($return, 'cn', _('Host name'));
		$this->addSimplePDFField($return, 'description', _('Description'));
		$this->addSimplePDFField($return, 'location', _('Location'));
		// managed by
		$managedBy = '';
		if (isset($this->attributes['managedBy'][0])) {
			$managedBy = getAbstractDN($this->attributes['managedBy'][0]);
			$this->addPDFKeyValue($return, 'managedBy', _('Managed by'), $managedBy);
		}
		return $return;
	}

	/**
	 * Formats a value in file time (100 ns since 1601-01-01).
	 *
	 * @param integer $value time value
	 * @return String formatted value
	 */
	private function formatFileTime($value) {
		if (empty($value) || ($value == '-1')) {
			return '';
		}
		$seconds = substr($value, 0, -7);
		$time = new DateTime('1601-01-01', new DateTimeZone('UTC'));
		$time->add(new DateInterval('PT' . $seconds . 'S'));
		$time->setTimezone(getTimeZone());
		return $time->format('Y-m-d H:i:s');
	}

	/**
	 * @inheritDoc
	 */
	public function getListAttributeDescriptions(ConfiguredType $type): array {
		return [
			'cn' => _('Host name'),
			'description' => _('Description'),
			'location' => _('Location'),
			'managedby' => _('Managed by'),
			'whencreated' => _('Creation time'),
			'whenchanged' => _('Change date'),
		];
	}

	/**
	 * @inheritDoc
	 */
	public function getListRenderFunction(string $attributeName): ?callable {
		if (($attributeName === 'whencreated') || ($attributeName === 'whenchanged')) {
			return function(array $entry, string $attribute): ?htmlElement {
				$value = ' - ';
				if (!empty($entry[$attribute][0])) {
					$value = formatLDAPTimestamp($entry[$attribute][0]);
				}
				return new htmlOutputText($value);
			};
		}
		elseif ($attributeName === 'managedby') {
			return function(array $entry, string $attribute): ?htmlElement {
				if (!empty($entry[$attribute][0])) {
					$value = $entry[$attribute][0];
					$typeManager = new TypeManager();
					$replaced = false;
					foreach ($typeManager->getConfiguredTypes() as $type) {
						if ((stripos($value, $type->getSuffix()) > 0) && !isAccountTypeHidden($type->getId())) {
							$value = '<a href="../account/edit.php?type=' . $type->getId() . '&amp;DN=\'' . $value . '\'">' . getAbstractDN($value) . '</a>';
							$replaced = true;
							break;
						}
					}
					if (!$replaced) {
						$value = getAbstractDN($value);
					}
					return new htmlDiv(null, new htmlOutputText($value, false), ['rightToLeftText']);
				}
				return null;
			};
		}
		return null;
	}

}
